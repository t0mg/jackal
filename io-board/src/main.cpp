#include <Arduino.h>
#include <Wire.h>
#include <movingAvg.h>
#include <Bounce2.h>
#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

// Disabling FM capacitor for now, it's not working great
// and it creates a lot of noise.
#ifdef ENABLE_FM_CAPACITOR
#include "FMCapacitor.h"
#endif

// #define DEBUG 1

// I2C Configuration
#define I2C_ADDRESS 0x02

// Pin Definitions
#define PIN_BTN 3             // Orange button
#define PIN_BAND 4            // Band select button
#define PIN_INPUT_SELECT 12   // Input select button
#define PIN_CONTROLS A3       // Multi-button analog input
#define PIN_TVTUNE_POT A0     // TV/VHF tuning pot
#define PIN_VOLUME_POT A1     // Volume pot
#define PIN_BRIGHTNESS_POT A2 // Brightness pot
#define PIN_TONE_POT A6       // Tone pot
#define LED_BUILTIN 13        // Built-in LED pin
#define PN532_TX 8            // TX pin connected to PN532 RXD (labeled SCL)
#define PN532_RX 9            // RX pin connected to PN532 TXD (labeled SDA)

// Button Setup
Bounce2::Button orangeBtn = Bounce2::Button();
Bounce2::Button bandSelectBtn = Bounce2::Button();
Bounce2::Button inputSelectBtn = Bounce2::Button();

// Moving Average Filters for Pots
movingAvg volPot(20); // 20-sample moving average
movingAvg tonePot(20);
movingAvg tvPot(40);
movingAvg brightnessPot(20);

// Control Button States
enum ControlValue
{
  NONE = 0,
  PREV = 1,
  PLAY = 2,
  NEXT = 3
};
ControlValue ctrlCommand = NONE;
bool pressingCtrlBtn = false;

#ifdef ENABLE_FM_CAPACITOR
FMCapacitor cap;
#endif

// I2C register (for future use)
volatile byte i2cRegister = 0xff;

// ControlValue lastSentCommand = NONE;
bool commandWasSent = true; // Start true so we don't send NONE initially
SoftwareSerial nfcSerial(PN532_RX, PN532_TX);
PN532_SWHSU pn532swhsu(nfcSerial);
PN532 nfc(pn532swhsu);
uint64_t lastNfcId = 0;
uint8_t lastUidLength = 0;
bool nfcInitialized = false;

// Delay between analog reads
unsigned long lastPotRead = 0;
const unsigned long POT_READ_INTERVAL = 20; // 20ms interval

// Delay between NFC checks
unsigned long lastNfcCheck = 0;
const unsigned long NFC_CHECK_INTERVAL = 1000; // Check every 1000ms

void i2cReceive(int bytesReceived)
{
  i2cRegister = Wire.read();
#ifdef DEBUG
  if (bytesReceived > 1)
  {
    // This is i2c "write" request
    Serial.print("received data: ");
    Serial.println(bytesReceived);
  }
#endif
  // Clean up I2C buffer
  while (Wire.available())
  {
    Wire.read();
  }
}

// Helper function to send mapped pot value
static auto sendMappedValue = [](uint16_t value, uint16_t maxInput)
{
  Wire.write(lowByte(map(min(value, maxInput), 0, maxInput, 0, 255)));
};

void i2cRequest()
{
  // Pack button states into a single byte
  byte pressedStates = (orangeBtn.isPressed() << 0) |
                       (bandSelectBtn.isPressed() << 1) |
                       (inputSelectBtn.isPressed() << 2);

  Wire.write(pressedStates);
  sendMappedValue(volPot.getAvg(), 870);
  sendMappedValue(tonePot.getAvg(), 870);
  sendMappedValue(tvPot.getAvg(), 810);
  sendMappedValue(brightnessPot.getAvg(), 870);
#ifdef ENABLE_FM_CAPACITOR
  sendMappedValue(cap.readFMValue(), 1023);
#else
  Wire.write(0);
#endif

  // Debug LED for command sending
  if (!commandWasSent && ctrlCommand != NONE)
  {
    digitalWrite(LED_BUILTIN, HIGH); // LED ON when sending new command
    // lastSentCommand = ctrlCommand;
    commandWasSent = true;
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW); // LED OFF when no new command
  }

  Wire.write(byte(ctrlCommand));

  // Send all 7 bytes of UID
  for (int i = 6; i >= 0; i--)
    Wire.write(byte((lastNfcId >> (i * 8)) & 0xFF));

#ifdef DEBUG
  Serial.print("I2C sending NFC UID: 0x");
  for (int i = 6; i >= 0; i--)
  {
    uint8_t byte = (lastNfcId >> (i * 8)) & 0xFF;
    if (byte < 0x10)
      Serial.print("0");
    Serial.print(byte, HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
}

void initNFC()
{
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
#ifdef DEBUG
  Serial.print("PN532 Firmware version: ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
#endif

  nfcInitialized = false;

  if (!versiondata)
  {
#ifdef DEBUG
    Serial.println("Didn't find PN532 board!");
#endif
    // while (1); // halt
  }
  else
  {
    nfc.SAMConfig();
    nfcInitialized = true;
#ifdef DEBUG
    Serial.println("NFC Ready");
#endif
  }
}

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Starting IO board");
#endif

  // Add a reset sequence
  pinMode(PN532_TX, OUTPUT);
  digitalWrite(PN532_TX, HIGH);
  delay(10);
  digitalWrite(PN532_TX, LOW);
  delay(100);
  digitalWrite(PN532_TX, HIGH);
  delay(10);

  nfcSerial.begin(115200);
  delay(200);

  // Configure buttons with pull-up resistors
  orangeBtn.attach(PIN_BTN, INPUT_PULLUP);
  orangeBtn.setPressedState(LOW);
  bandSelectBtn.attach(PIN_BAND, INPUT_PULLUP);
  bandSelectBtn.setPressedState(LOW);
  inputSelectBtn.attach(PIN_INPUT_SELECT, INPUT_PULLUP);
  inputSelectBtn.setPressedState(LOW);

  // Initialize moving averages
  volPot.begin();
  tonePot.begin();
  tvPot.begin();
  brightnessPot.begin();

  // Configure additional inputs
  pinMode(PIN_CONTROLS, INPUT);
  // pinMode(PIN_INPUT_SELECT, INPUT_PULLUP);

#ifdef ENABLE_FM_CAPACITOR
  // Initialize FM capacitor
  cap.setup();
#endif

  // Initialize NFC
  initNFC();

  // Configure I2C
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(i2cReceive);
  Wire.onRequest(i2cRequest);

  // Add with other pin configurations
  pinMode(LED_BUILTIN, OUTPUT);
}

void readControlBtn()
{
  int controls = analogRead(PIN_CONTROLS);

  // Reset if no button pressed
  if (controls < 5)
  {
    pressingCtrlBtn = false;
    ctrlCommand = NONE;
    return;
  }

  // Only process new button presses
  if (!pressingCtrlBtn)
  {
    pressingCtrlBtn = true;
    commandWasSent = false; // New command needs to be sent

    if (controls > 195)
    {
      return; // Multiple buttons pressed, ignore
    }
    else if (controls > 175)
      ctrlCommand = PLAY; // When powered separately from the main board the threshold is lower (160)
    else if (controls > 100)
      ctrlCommand = PREV;
    else if (controls > 20)
      ctrlCommand = NEXT;
  }
}

void checkNFC()
{
  if (!nfcInitialized)
  {
#ifdef DEBUG
    Serial.println("NFC not initialized, initializing...");
#endif
    initNFC();
    return;
  }

  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 90);
  lastNfcId = 0; // Clear before setting, in case of failure we'll send 0x0000000000000000

  if (success)
  {
#ifdef DEBUG
    Serial.print("Found tag! Raw bytes: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      if (uid[i] < 0x10)
        Serial.print("0");
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
#endif
    // Store the UID length and full ID
    lastUidLength = uidLength;
    for (uint8_t i = 0; i < uidLength && i < 7; i++)
    {
      lastNfcId = (lastNfcId << 8) | uid[i];
    }

    // Left-align the UID in the 56-bit field (7 bytes)
    if (uidLength < 7)
    {
      lastNfcId <<= (7 - uidLength) * 8;
    }

#ifdef DEBUG
    Serial.print("Packed lastNfcId: 0x");
    for (int i = 6; i >= 0; i--)
    {
      uint8_t byte = (lastNfcId >> (i * 8)) & 0xFF;
      if (byte < 0x10)
        Serial.print("0");
      Serial.print(byte, HEX);
    }
    Serial.println();
#endif
  }
}

void loop()
{
  // Update button states
  orangeBtn.update();
  bandSelectBtn.update();
  inputSelectBtn.update();

#ifdef DEBUG
  static uint8_t debugCounter = 0;
  if (++debugCounter >= 100)
  { // Only print every 10th iteration
    // Debug print button states
    Serial.print("Buttons - Orange: ");
    Serial.print(orangeBtn.isPressed());
    Serial.print(" Band: ");
    Serial.print(bandSelectBtn.isPressed());
    Serial.print(" Input: ");
    Serial.print(inputSelectBtn.isPressed());
    Serial.print(" Ctrl: ");
    Serial.print(ctrlCommand);

    // Add pot values to debug output
    Serial.print(" | Pots - Vol: ");
    Serial.print(volPot.getAvg());
    Serial.print(" Tone: ");
    Serial.print(tonePot.getAvg());
    Serial.print(" TV: ");
    Serial.print(tvPot.getAvg());
#ifdef ENABLE_FM_CAPACITOR
    Serial.print(" FM: ");
    Serial.print(cap.readFMValue());
#endif
    Serial.print(" Brightness: ");
    Serial.println(brightnessPot.getAvg());
    debugCounter = 0;
  }
#endif

  unsigned long currentMillis = millis();
  if (currentMillis - lastPotRead >= POT_READ_INTERVAL) {
    lastPotRead = currentMillis;
    
    volPot.reading(analogRead(PIN_VOLUME_POT));
    tvPot.reading(analogRead(PIN_TVTUNE_POT));
    tonePot.reading(analogRead(PIN_TONE_POT));
    brightnessPot.reading(analogRead(PIN_BRIGHTNESS_POT));
    readControlBtn();
  }

  if (currentMillis - lastNfcCheck >= NFC_CHECK_INTERVAL) {
    lastNfcCheck = currentMillis;
    checkNFC();
  }
}
