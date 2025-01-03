#define DEBUG
#define ENABLE_MTP

#include <Audio.h>
#include <Wire.h>
#include <TimeLib.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MTP_Teensy.h>

#include "Log.h"
#include "FFT.h"
#include "FM.h"
#include "Display.h"
#include "I2C.h"
#include "AudioMode.h"
#include "Recorder.h"
#include "sound/boot_sound.h"
#include "sound/boop.h"
#include "sprites/hold_to_record.h"
#include "AudioSystem.h"
#include "AudioModeController.h"
#include "AudioModeControllerNull.h"
#include "AudioModeControllerBluetooth.h"
#include "AudioModeControllerRadio.h"
#include "AudioModeControllerSDPlayer.h"
#include "AudioModeControllerSDRecorder.h"
#include "AudioModeControllerNFCPlayer.h"
#include "AudioModeControllerTimeSetup.h"

#define PIN_VUMETER 14
#define PIN_BRIGHTNESS 2
#define SDCARD_CS_PIN 10
#define SDCARD_MOSI_PIN 11
#define SDCARD_SCK_PIN 13

#define QUICK_BOOT_THRESHOLD 5 // 5 seconds

Display display;
FFT fft;
I2C i2c;
I2CTimer timer = i2c.getTimer();
FM radio(&timer);

AudioSystem audioSystem;
AudioAnalyzePeak *currentPeak = audioSystem.getPlaybackPeak();
Recorder recorder(*audioSystem.getWavPlayer(), *audioSystem.getRecordQueue());

AudioModeControllerNull nullController(audioSystem);
AudioModeController *audioController = &nullController;

bool needsTimeSetup = false;

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

uint32_t mtpCheckTime;
void setMTPdeviceChecks(bool enable)
{
  if (enable)
  {
    MTP.storage()->set_DeltaDeviceCheckTimeMS(mtpCheckTime);
    LOG("Enabled MTP device checks");
  }
  else
  {
    MTP.storage()->set_DeltaDeviceCheckTimeMS((uint32_t)-1);
    LOG("Disabled MTP device checks");
  }
}

AudioMode computeMode(bool inputButtonPressed, bool bandButtonPressed)
{
  LOGF("Input button: %d, Band button: %d\n", inputButtonPressed, bandButtonPressed);
  if (inputButtonPressed)
  {
    // Input button pressed = Bluetooth or Radio modes
    return bandButtonPressed ? MODE_RADIO : MODE_BLUETOOTH;
  }
  else
  {
    // Input button released = SD modes
    return bandButtonPressed ? MODE_SD_PLAYBACK : MODE_SD_RECORDER;
  }
}

void updateMode(AudioMode newMode)
{
  // Check if time setup is complete
  if (audioController->getMode() == MODE_TIME_SETUP)
  {
    AudioModeControllerTimeSetup *timeSetup = (AudioModeControllerTimeSetup *)audioController;
    needsTimeSetup = !timeSetup->isTimeSetComplete();
  }

  if (needsTimeSetup && newMode != MODE_TIME_SETUP)
  {
    LOG("Time setup is required, ignoring mode switch");
    return;
  }

  AudioMode currentMode = audioController->getMode();
  LOGF("Update mode, currentMode: %d, newMode: %d\n", currentMode, newMode);

  static elapsedMillis lastModeSwitch = 0;

  if (currentMode != MODE_UNKNOWN)
  {
    if (lastModeSwitch < 500)
      return; // Debounce mode switches
    lastModeSwitch = 0;

    // Only update if mode actually changed
    if (currentMode == newMode && newMode != MODE_NFC_PLAYBACK)
      return;
  }

  LOGF("Switching to mode %d\n", newMode);

  audioController->exit();

  // Delete previous controller if it's not the null controller
  if (audioController != &nullController)
  {
    delete audioController;
  }

  AudioMemoryUsageMaxReset();

  display.clearTemporaryMetadata();
  display.clearMainArea();
  setMTPdeviceChecks(newMode == MODE_RADIO || newMode == MODE_BLUETOOTH);

  switch (newMode)
  {
  case MODE_BLUETOOTH:
    audioController = new AudioModeControllerBluetooth(display, i2c, audioSystem);
    break;
  case MODE_RADIO:
    audioController = new AudioModeControllerRadio(display, i2c, audioSystem, radio);
    break;
  case MODE_SD_PLAYBACK:
    audioController = new AudioModeControllerSDPlayer(display, i2c, audioSystem, recorder);
    break;
  case MODE_SD_RECORDER:
    audioController = new AudioModeControllerSDRecorder(display, i2c, audioSystem, recorder);
    break;
  case MODE_NFC_PLAYBACK:
    audioController = new AudioModeControllerNFCPlayer(display, i2c, audioSystem, recorder);
    break;
  case MODE_TIME_SETUP:
    audioController = new AudioModeControllerTimeSetup(display, i2c, audioSystem);
    break;
  default:
    audioController = &nullController;
    break;
  }

  fft.updatePalette(audioController->getTheme().fftFront, audioController->getTheme().fftBack, audioController->getTheme().fftMain);

  currentPeak = audioController->getPeak();

  audioController->enter();

  LOGF("Mode switch complete, now in mode: %d\n", audioController->getMode());
}

int getMappedValue(float val, const int map[], int length)
{
  if (val <= .01)
    return map[0];
  if (val >= .99)
    return map[length - 1];
  return map[(int)floor(val / (1.0 / length))];
}

// ------------------ Button handlers --------------------- //

void onInputButton(bool pressed)
{
  LOG("Input select button ");
  bool bandPressed = i2c.getIOState().buttonStates & BAND_BTN;
  AudioMode newMode = computeMode(pressed, bandPressed);
  updateMode(newMode);
}

void onBandButton(bool pressed)
{
  LOG("Band button ");
  bool inputPressed = i2c.getIOState().buttonStates & INPUT_BTN;
  AudioMode newMode = computeMode(inputPressed, pressed);
  updateMode(newMode);
}

void onOrangeButton(bool pressed)
{
  LOG("Orange button ");
  LOG(pressed ? "pressed" : "released");
  if (pressed)
  {
    audioSystem.getMemoryPlayer()->play(boop);
  }
  audioController->handleOrangeButton(pressed);
}

void onControl(ControlCommand cmd)
{
  audioController->handleControl(cmd);
}

void onNfcTag(String uid)
{
  if (uid == "000000000000FF")
  {
    LOG("NFC Tag removed - reverting to previous mode");
    bool inputPressed = i2c.getIOState().buttonStates & INPUT_BTN;
    bool bandPressed = i2c.getIOState().buttonStates & BAND_BTN;
    AudioMode newMode = computeMode(inputPressed, bandPressed);
    updateMode(newMode);
  }
  else
  {
    LOGF("NFC Tag detected - UID: %s\n", uid.c_str());
    updateMode(MODE_NFC_PLAYBACK);
  }
}

// ------------------ Setup --------------------- //

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  // while (!Serial);
#endif
  LOG("========== Jackal2024 ==========");

  SNVS_LPCR |= (1 << 24); // Enable NVRAM

  LOG("Setup time provider");
  setSyncProvider(getTeensy3Time);

  if (timeStatus() != timeSet || (hour() == 0 && minute() == 0 && day() == 1 && month() == 1))
  {
    LOG("RTC was invalid - will display time setup screen");
    needsTimeSetup = true;
  }

  bool rtcValid = (timeStatus() == timeSet);
  // Only check quick boot if RTC is valid and time isn't 00:00
  bool quickBoot = rtcValid && !needsTimeSetup && (now() - SNVS_LPGPR1) < QUICK_BOOT_THRESHOLD;

  LOGF("RTC valid: %d, Quick boot: %d, last update: %d, now: %d\n",
       rtcValid, quickBoot, SNVS_LPGPR1, now());

  LOG("Setup pins");
  pinMode(PIN_VUMETER, OUTPUT);
  pinMode(PIN_BRIGHTNESS, OUTPUT);

  LOG("Init display");
  display.init();
  display.clear();
  display.update();

  LOG("Init SD card");
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN)))
  {
    LOG("Unable to access the SD card");
  }

  // Optimize SPI for SD card
  SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));

  LOG("Init audio board");
  audioSystem.init();

  // audioController->setMixerGains();

  LOG("Init FFT");
  fft.init(audioSystem.getFFT());

  LOGF("Quick boot: %d, last update: %d, now: %d\n", quickBoot, SNVS_LPGPR1, now());
  if (!quickBoot)
  {
    // Normal boot sequence
    LOG("Start boot animation");
    analogWrite(PIN_BRIGHTNESS, 240);
    audioSystem.getOutputAmp()->gain(0.04);
    audioSystem.getMemoryPlayer()->play(boot_sound);
    audioSystem.getBitcrusher()->bits(14);
    audioSystem.getBitcrusher()->sampleRate(5512);
    display.drawSplash();

    elapsedMillis splashTimer = 0;
    while (splashTimer < 5800) // animationEnd in Display.cpp
    {
      display.update();
      yield();
    }

    audioSystem.getBitcrusher()->bits(16);
    audioSystem.getBitcrusher()->sampleRate(44100);
  }
  else
  {
    LOG("Quick boot sequence");
    analogWrite(PIN_BRIGHTNESS, 240);
    audioSystem.getOutputAmp()->gain(0);
  }

  display.clear();
  display.update();

  LOG("Setup I2C");
  i2c.init();

  LOG("Setup FM");
  radio.init();

  LOG("Init start mode");
  while (!i2c.requestDataFromIO(false))
  {
    delay(50);
  }

  LOG("Enable MTP");
  MTP.begin();
  MTP.addFilesystem(SD, "Media");
  mtpCheckTime = MTP.storage()->get_DeltaDeviceCheckTimeMS();

  if (needsTimeSetup)
  {
    updateMode(MODE_TIME_SETUP);
  }
  else
  {
    // Read IO state after I2C is fully initialized
    IOState initialState = i2c.getIOState();
    bool bandPressed = initialState.buttonStates & BAND_BTN;
    bool inputPressed = initialState.buttonStates & INPUT_BTN;

    LOGF("Initial button states - Input: %d, Band: %d\n", inputPressed, bandPressed);

    updateMode(computeMode(inputPressed, bandPressed));
    LOGF("Initial mode set to: %d\n", audioController->getMode());
  }

  LOG("Attach callbacks");
  i2c.setOrangeButtonCallback(onOrangeButton);
  i2c.setBandButtonCallback(onBandButton);
  i2c.setInputButtonCallback(onInputButton);
  i2c.setControlCallback(onControl);
  i2c.setNfcTagCallback(onNfcTag);

  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
}

// ------------------ Main loop --------------------- //

elapsedMillis msecs;
elapsedMillis clockTimer;

// Audio processing constants
static const int SAMPLE_RATES[] = {2756, 5512, 5512, 5512, 11025, 11025, 22050, 22050, 44100};
static const int SAMPLE_RATES_LENGTH = 9;

static const int BIT_DEPTHS[] = {8, 10, 12, 14, 16};
static const int BIT_DEPTHS_LENGTH = 5;

void loop()
{
  i2c.loop();

  if (!recorder.isRecording() && !recorder.isPlaying())
  {
    MTP.loop();
  }

  audioController->loop();

  // Main audio/display loop
  if (msecs > 30)
  {
    msecs = 0;
    audioController->updateOutputVolume();
    analogWrite(PIN_BRIGHTNESS, i2c.getIOState().brightness);
    // Update clock every 300ms
    if (clockTimer >= 300 && !needsTimeSetup)
    {
      clockTimer = 0;
      display.updateClock();
      SNVS_LPGPR1 = now();
    }

    if (i2c.getIOState().volume == 0 && audioController->getMode() != MODE_SD_RECORDER)
    {
      analogWrite(PIN_VUMETER, 0);
    }
    else if (currentPeak && currentPeak->available())
    {
      // Modified VU meter curve (Gaussian) to be more sensitive to lower values.
      float mean = 0.7;  // Lower mean to shift sensitivity curve left
      float sigma = 0.5; // Tighter sigma for steeper response
      float peak = currentPeak->read();
      float val = exp(-0.5 * pow((pow(peak, 0.5) - mean) / sigma, 2.)) - 0.381138;
      int monoPeak = min(val * 25.0, 255);
      analogWrite(PIN_VUMETER, monoPeak);
    }

    float tonePotVal = map((float)i2c.getIOState().tone, 0.f, 255.f, 0.f, 1.f);
    int mappedVal = getMappedValue(tonePotVal, BIT_DEPTHS, BIT_DEPTHS_LENGTH);
    int mappedVal2 = getMappedValue(tonePotVal, SAMPLE_RATES, SAMPLE_RATES_LENGTH);

    audioSystem.getBitcrusher()->bits(mappedVal);
    audioSystem.getBitcrusher()->sampleRate(mappedVal2);

    if (!needsTimeSetup && fft.available())
    {
      display.clearMainArea();
      display.tft.setClipRect(0, 40, 320, 140);
      uint8_t tuning = i2c.getIOState().tuning;
      fft.drawHistory(&display, tuning);
      fft.drawNewLevels(&display, tuning);
      display.tft.setClipRect();
    }

    audioController->frameLoop();

    if (recorder.isRecording())
    {
      display.updateAsync();
    }
    else
    {
      display.update();
    }
  }
}
