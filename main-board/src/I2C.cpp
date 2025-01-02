#include "I2C.h"
#include "Log.h"
#include <Wire.h>

void I2C::init()
{
  Wire1.setSCL(SCL_PIN);
  Wire1.setSDA(SDA_PIN);
  Wire1.begin();
  Wire1.setClock(100000);
  delay(300); // Give devices time to initialize

  LOG_I2C_MSG("LOG_I2C Debug: scanning I2C bus...");
#if LOG_I2C
  for (byte address = 1; address < 127; address++)
  {
    Wire1.beginTransmission(address);
    byte error = Wire1.endTransmission();
    if (error == 0)
    {
      Serial.print("Device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
#endif

  i2cTimer.begin();
  i2cTimer.resetTimeout();
}

bool paused = false;
bool wasPlaying = false;
void I2C::loop()
{
  static uint8_t btFailCount = 0;
  static uint8_t ioFailCount = 0;
  static elapsedMillis lastWireReset = 0;
  static elapsedMillis lastSuccessfulComm = 0;
  const unsigned long WIRE_RESET_COOLDOWN = 1000;

  i2cTimer.update();

  // Check for audio state change and maintain I2C when stopped
  bool isPlaying = (currentMode_ == MODE_BLUETOOTH && metadata_.isPlaying);
  if (wasPlaying && !isPlaying)
  {
    // Audio just stopped - force a full I2C reset sequence
    LOG_I2C_MSG("Audio stopped - resetting I2C");
    Wire1.end(); // Completely shut down I2C
    yield();
    Wire1.begin(); // Restart I2C fresh
    lastWireReset = 0;
    btFailCount = 0;
    ioFailCount = 0;
  }
  wasPlaying = isPlaying;

  if (i2cTimer.hasTimeout())
  {
    LOG_I2C_MSG("I2C timeout");
    if (lastWireReset >= WIRE_RESET_COOLDOWN)
    {
      LOG_I2C_MSGF("Time since last successful comm: %d ms\n", (int)lastSuccessfulComm);
      LOG_I2C_MSG("Resetting I2C bus");
      i2cTimer.startManualOperation();
      Wire1.begin();
      lastWireReset = 0;
      btFailCount = 0;
      ioFailCount = 0;
      i2cTimer.releaseBus();
    }
    i2cTimer.resetTimeout();
    return;
  }

  // Handle IO polling first
  if (i2cTimer.shouldPollIO())
  {
    // LOG_I2C_MSG("Polling IO");
    if (requestDataFromIO(false))
    {
      i2cTimer.resetTimeout();
      i2cTimer.markIOPolled();
      i2cTimer.releaseBus();
      ioFailCount = 0;
      lastSuccessfulComm = 0; // Reset timer on success
      // Process control changes
      if (!ioState_.controlProcessed && controlCallback_)
      {
        controlCallback_(ioState_.control);
        ioState_.controlProcessed = true;
      }
    }
    else
    {
      ioFailCount++;
      LOG_I2C_MSGF("IO poll failed (%d times, %d ms since last success)\n",
                   ioFailCount, (int)lastSuccessfulComm);
      if (ioFailCount >= 3 && lastWireReset >= WIRE_RESET_COOLDOWN)
      {
        LOG_I2C_MSG("Multiple IO failures, resetting bus");
        i2cTimer.startManualOperation();
        Wire1.begin();
        lastWireReset = 0;
        ioFailCount = 0;
        btFailCount = 0;
        i2cTimer.releaseBus();
      }
      yield();
    }
  }

  // Handle BT commands and polling
  if (pendingBTCommand_ != 0)
  {
    static uint8_t cmdRetryCount = 0;
    static elapsedMillis lastRetryTime = 0;
    const unsigned long RETRY_DELAY = 100;

    if (lastRetryTime >= RETRY_DELAY)
    {
      LOG_BT_MSGF("Attempting to send command: %c (retry: %d)\n", pendingBTCommand_, cmdRetryCount);
      i2cTimer.startManualOperation();
      Wire1.beginTransmission(BT_MODULE_I2C_ADDRESS);
      Wire1.write(pendingBTCommand_);
      byte error = Wire1.endTransmission();
      i2cTimer.releaseBus();

      if (error == 0)
      {
        LOG_BT_MSG("Command sent successfully");
        pendingBTCommand_ = 0;
        cmdRetryCount = 0;
        btFailCount = 0;
      }
      else
      {
        cmdRetryCount++;
        LOG_BT_MSGF("Command failed (error: %d)\n", error);

        if (cmdRetryCount >= 3)
        {
          LOG_BT_MSG("Max retries reached, dropping command");
          pendingBTCommand_ = 0;
          cmdRetryCount = 0;
        }
      }
      lastRetryTime = 0;
    }
  }
  else if (i2cTimer.shouldPollBluetooth() && currentMode_ == MODE_BLUETOOTH)
  {
    LOG_BT_MSG("Polling Bluetooth");
    String data = requestDataFromBluetooth();
    if (data.length() > 0)
    {
      i2cTimer.resetTimeout();
      i2cTimer.markBTPolled();
      btFailCount = 0;
      lastSuccessfulComm = 0; // Reset timer on success
      i2cTimer.releaseBus();
    }
    else
    {
      btFailCount++;
      LOG_BT_MSGF("BT poll failed (%d times, %d ms since last success)\n",
                  btFailCount, (int)lastSuccessfulComm);
      if (btFailCount >= 3 && lastWireReset >= WIRE_RESET_COOLDOWN)
      {
        LOG_BT_MSG("Multiple BT failures, resetting bus");
        i2cTimer.startManualOperation();
        Wire1.begin();
        lastWireReset = 0;
        btFailCount = 0;
        ioFailCount = 0;
        i2cTimer.releaseBus();
      }
    }
  }
}

// Helper function to process analog values with spike filtering
static bool isWithinTolerance(byte a, byte b, byte tolerance = 2)
{
  return abs(a - b) <= tolerance;
}

static void processAnalogValue(byte newValue, byte &currentValue, int8_t &skippedValue, bool warmingUp = false, int threshold = 10, bool allowDecrease = false)
{

  // Allow first reading to pass through
  if (warmingUp || (allowDecrease && newValue < currentValue)) {
      currentValue = newValue;
      LOG_I2C_MSGF("Initial value set: %d\n", newValue);
      return;
  }

  if (newValue != currentValue)
  {
    if (newValue != 0 && abs(newValue - currentValue) > threshold)
    {
      if (skippedValue == -1)
      {
        // First detection of large jump - skip this value
        // LOG_I2C_MSGF("Skipping large value change: %d -> %d\n", currentValue, newValue);
        skippedValue = newValue;
      }
      else if (!isWithinTolerance(newValue, skippedValue) && newValue != currentValue)
      {
        // New value different from both current and skipped - accept it
        // LOG_I2C_MSGF("Accepting value after skip: %d -> %d\n", currentValue, newValue);
        currentValue = newValue;
        skippedValue = -1;
      }
      else
      {
        // LOG_I2C_MSGF("Skipping large value change AGAIN: %d -> %d\n", currentValue, newValue); 
        skippedValue = newValue;
      }
    }
    else
    {
      // Small changes and jumps to 0 accepted immediately
      // LOG_I2C_MSGF("Accepting small value change: %d -> %d\n", currentValue, newValue);
      currentValue = newValue;
      skippedValue = -1;
    }
  }
}

bool I2C::requestDataFromIO(bool isRetry)
{
  static const int IO_DATA_LENGTH = 13;

  // Track skipped values for each analog input (-1 means no skip in progress)
  static struct
  {
    int8_t volume = -1;
    int8_t tone = -1;
    int8_t tuning = -1;
    int8_t brightness = -1;
    int8_t fmValue = -1;
  } skippedValues;

  Wire1.requestFrom(static_cast<int>(IO_BOARD_I2C_ADDRESS), IO_DATA_LENGTH);

  if (Wire1.available() >= IO_DATA_LENGTH)
  {
    byte newButtons = Wire1.read();
    byte rawVolume = Wire1.read();
    byte rawTone = Wire1.read();
    byte rawTuning = Wire1.read();
    byte rawBrightness = Wire1.read();
    byte newFmValue = Wire1.read();
    ControlCommand newControl = static_cast<ControlCommand>(Wire1.read());
    
    // Read NFC UID (7 bytes)
    String newNfcUidString = "";
    // LOG_I2C_MSG("I2C receiving NFC UID");
    for (int i = 0; i < 7; i++) {
        uint8_t b = Wire1.read();
        // Add leading zero if needed
        if (b < 0x10) {
            newNfcUidString += "0";
        }
        newNfcUidString += String(b, HEX);
    }
    newNfcUidString.toUpperCase();  // Convert to uppercase for consistency

    if (newNfcUidString != ioState_.nfcUidString) {
        if (newNfcUidString == "000000000000FF" || newNfcUidString == "00000000000000") {  // No tag detected
            if (!noTagTimerStarted) {
                noTagTimer = 0;
                noTagTimerStarted = true;
            }
            else if (noTagTimer >= NO_TAG_DEBOUNCE_TIME) {
                // Only update state and trigger callback after debounce period
                LOG_I2C_MSGF("NFC UID changed (no tag): %s\n", newNfcUidString.c_str());
                ioState_.nfcUidString = newNfcUidString;
                if (nfcTagCallback_) {
                    nfcTagCallback_(newNfcUidString);
                }
                noTagTimerStarted = false;
            }
        } else {
            // Regular tag detected - process immediately
            LOG_I2C_MSGF("NFC UID changed: %s\n", newNfcUidString.c_str());
            ioState_.nfcUidString = newNfcUidString;
            if (nfcTagCallback_) {
                nfcTagCallback_(newNfcUidString);
            }
            noTagTimerStarted = false;
        }
    } else {
        noTagTimerStarted = false;
    }

    // Process individual button state changes
    if (newButtons != ioState_.buttonStates)
    {
      // Orange button
      LOG_I2C_MSGF("Orange button state: %d\n", newButtons & ORANGE_BTN);
      if (orangeButtonCallback_)
      {
        bool newOrangeState = newButtons & ORANGE_BTN;
        bool oldOrangeState = ioState_.buttonStates & ORANGE_BTN;
        if (newOrangeState != oldOrangeState)
        {
          orangeButtonCallback_(newOrangeState);
        }
      }

      // Band button
      LOG_I2C_MSGF("Band button state: %d\n", newButtons & BAND_BTN);
      if (bandButtonCallback_)
      {
        bool newBandState = newButtons & BAND_BTN;
        bool oldBandState = ioState_.buttonStates & BAND_BTN;
        if (newBandState != oldBandState)
        {
          bandButtonCallback_(newBandState);
        }
      }

      // Input button
      LOG_I2C_MSGF("Input button state: %d\n", newButtons & INPUT_BTN);
      if (inputButtonCallback_)
      {
        bool newInputState = newButtons & INPUT_BTN;
        bool oldInputState = ioState_.buttonStates & INPUT_BTN;
        if (newInputState != oldInputState)
        {
          inputButtonCallback_(newInputState);
        }
      }
    }

    // Reset controlProcessed flag if control command changes
    if (newControl != ioState_.control)
    {
      ioState_.controlProcessed = false;
      ioState_.control = newControl;
      LOG_I2C_MSGF("Control command changed: %d\n", newControl);
    }

    // Process all analog values with spike filtering
    bool isWarmingUp = i2cTimer.isWarmingUp();
    processAnalogValue(rawVolume, ioState_.volume, skippedValues.volume, isWarmingUp);
    processAnalogValue(rawTone, ioState_.tone, skippedValues.tone, isWarmingUp);
    processAnalogValue(rawTuning, ioState_.tuning, skippedValues.tuning, isWarmingUp, 20);
    processAnalogValue(rawBrightness, ioState_.brightness, skippedValues.brightness, isWarmingUp, 20);

    // Process buttons and store other values...
    ioState_.buttonStates = newButtons;
    ioState_.fmValue = newFmValue; // TODO: debounce/process?
    // ioState_.control = newControl;
    //ioState_.controlProcessed = false; // Mark new control as unprocessed

    return true;
  }

  return false;
}

String I2C::requestDataFromBluetooth()
{
  static elapsedMillis lastPollTime = 0;
  const unsigned int MIN_POLL_INTERVAL = 50;

  if (lastPollTime < MIN_POLL_INTERVAL)
  {
    yield();
    return "";
  }
  lastPollTime = 0;

  // Request data
  size_t bytesRequested = Wire1.requestFrom(static_cast<int>(BT_MODULE_I2C_ADDRESS), MAX_BT_DATA_LENGTH);
  LOG_BT_MSGF("Requested %d bytes\n", bytesRequested);

  if (bytesRequested != MAX_BT_DATA_LENGTH)
  {
    LOG_BT_MSGF("Request failed, got %d bytes\n", bytesRequested);
    return "";
  }

  static char buffer[MAX_BT_DATA_LENGTH];
  memset(buffer, 0, MAX_BT_DATA_LENGTH);

  // Read available bytes with timeout
  size_t bytesRead = 0;
  elapsedMillis readTimeout = 0;
  const unsigned long READ_TIMEOUT = 5; // 5ms timeout

  while (readTimeout < READ_TIMEOUT && bytesRead < MAX_BT_DATA_LENGTH)
  {
    if (Wire1.available())
    {
      buffer[bytesRead++] = Wire1.read();
    }
    if (bytesRead % 32 == 0)
      yield();
  }

  LOG_BT_MSGF("Read %d bytes\n", bytesRead);

  if (bytesRead == 0)
  {
    LOG_BT_MSG("No bytes read");
    return "";
  }

  // Ensure null termination
  buffer[MAX_BT_DATA_LENGTH - 1] = '\0';

  // Parse metadata
  char *dataStart = strchr(buffer, '|');
  if (dataStart)
  {
    parseMetadata_(dataStart + 1);
  }
  else
  {
    LOG_BT_MSG("Corrupted data received:");
    for (size_t j = 0; j < min(bytesRead, (size_t)16); j++)
    {
      LOG_BT_MSGF("%02X ", buffer[j]);
    }
    LOG_BT_MSG("");
    return "";
  }

  return buffer;
}

void I2C::parseMetadata_(const String &data)
{
  String title, artist, playingState, connectionStatus;
  bool hasChanges = false;

  // Add debug logging for raw data
  LOG_BT_MSG("Raw metadata: " + data);

  size_t pos = 0;
  while (pos < data.length())
  {
    // Safeguard against buffer overrun
    if (pos >= data.length())
      break;

    char type = data[pos++];
    int delimPos = data.indexOf('|', pos);

    // Handle case where this is the last field
    String value;
    if (delimPos != -1)
    {
      value = data.substring(pos, delimPos);
    }
    else
    {
      value = data.substring(pos);
      // Trim any null characters or whitespace
      value.trim();
    }

    // Log each field for debugging
    LOG_BT_MSGF("Parsing field: %c = '%s'\n", type, value.c_str());

    switch (type)
    {
    case 'T':
      if (value != metadata_.title)
      {
        metadata_.title = value;
        hasChanges = true;
      }
      break;
    case 'A':
      if (value != metadata_.artist)
      {
        // Add extra logging for artist field
        LOG_BT_MSG("Previous artist: '" + metadata_.artist + "'");
        LOG_BT_MSG("New artist: '" + value + "'");
        metadata_.artist = value;
        hasChanges = true;
      }
      break;
    case 'S':
      playingState = value;
      break;
    case 'C':
      if (value != metadata_.deviceName)
      {
        metadata_.deviceName = value;
        metadata_.isConnected = (value != "disconnected");
        hasChanges = true;
      }
      break;
    }

    if (delimPos == -1)
      break;
    pos = delimPos + 1;
  }

  // Handle playing state separately since it needs special processing
  if (playingState.length() > 0)
  {
    bool newPlayingState = (playingState == "PLAYING");
    if (newPlayingState != metadata_.isPlaying)
    {
      metadata_.isPlaying = newPlayingState;
      hasChanges = true;
      LOG_BT_MSG("Play state changed to: " + String(newPlayingState ? "Playing" : "Not Playing"));
    }
    metadata_.awaitingUpdate = false; // Clear the waiting flag when we get a status update
    LOG_BT_MSG("Cleared awaiting update flag");
  }

  if (hasChanges)
  {
    metadata_.updated = true;
    LOG_BT_MSG("Title: " + metadata_.title);
    LOG_BT_MSG("Artist: " + metadata_.artist);
    LOG_BT_MSG("Playing: " + String(metadata_.isPlaying ? "Yes" : "No"));
    LOG_BT_MSG("Connected: " + String(metadata_.isConnected ? "Yes" : "No"));
    LOG_BT_MSG("Device: " + metadata_.deviceName);
  }
}

void I2C::queueBTCommand(char cmd)
{
  pendingBTCommand_ = cmd;
}

void I2C::btPlay()
{
  queueBTCommand('p');
}

void I2C::btPause()
{
  queueBTCommand('s');
}

void I2C::btPrevious()
{
  queueBTCommand('r');
}

void I2C::btNext()
{
  queueBTCommand('n');
}
