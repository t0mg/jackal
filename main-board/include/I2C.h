#pragma once

#include <Arduino.h>
#include "I2CTimer.h"
#include "AudioMode.h"

#define IO_BOARD_I2C_ADDRESS 0x02
#define BT_MODULE_I2C_ADDRESS 0x03
#define SDA_PIN 17
#define SCL_PIN 16

#define MAX_BT_DATA_LENGTH 136 // Maximum length of data string
#define BT_COMMAND_COOLDOWN 500

// Add this struct at the top of the file, before the I2C class
struct Metadata
{
  String title;
  String artist;
  bool isPlaying;
  bool updated;
  bool awaitingUpdate;
  unsigned long lastCommandTime;
  bool isConnected;
  String deviceName;

  Metadata() : title(""), artist(""), isPlaying(false), updated(false), awaitingUpdate(false), lastCommandTime(0), isConnected(false), deviceName("disconnected") {}
};

#define COMMAND_TIMEOUT 3000 // 3 seconds timeout for command response

enum ControlCommand
{
  NONE = 0,
  PREV = 1,
  PLAY = 2,
  NEXT = 3
};
struct IOState
{
  byte buttonStates;
  uint8_t volume;
  uint8_t tone;
  uint8_t tuning;
  uint8_t brightness;
  uint8_t fmValue;
  ControlCommand control;
  bool controlProcessed;
  String nfcUidString;

  IOState() : buttonStates(0), volume(0), tone(0), tuning(0),
              brightness(0), fmValue(0), control(NONE),
              controlProcessed(false) {}
};

// Add these button definitions
enum ButtonMask
{
  ORANGE_BTN = (1 << 0),
  BAND_BTN = (1 << 1),
  INPUT_BTN = (1 << 2)
};

class I2C
{
public:
  static I2C &null()
  {
    static I2C instance;
    return instance;
  }
  void init(void);
  void loop(void);
  I2CTimer& getTimer() { return i2cTimer; }
  void btPlay();
  void btPause();
  void btPrevious();
  void btNext();
  String requestDataFromBluetooth();
  bool requestDataFromIO(bool isRetry);
  void queueBTCommand(char cmd);

  // Add getter for metadata
  const Metadata &getMetadata() const { return metadata_; }
  bool hasNewMetadata()
  {
    if (metadata_.updated)
    {
      metadata_.updated = false;
      return true;
    }
    return false;
  }

  // Callback types
  using ButtonCallback = void (*)(bool pressed);
  using ControlCallback = void (*)(ControlCommand cmd);
  using NfcTagCallback = void (*)(String uid);
  void setOrangeButtonCallback(ButtonCallback cb) { orangeButtonCallback_ = cb; }
  void setBandButtonCallback(ButtonCallback cb) { bandButtonCallback_ = cb; }
  void setInputButtonCallback(ButtonCallback cb) { inputButtonCallback_ = cb; }
  void setControlCallback(ControlCallback cb) { controlCallback_ = cb; }
  void setNfcTagCallback(NfcTagCallback cb) { nfcTagCallback_ = cb; }

  const IOState &getIOState() const { return ioState_; }
  void setCurrentMode(AudioMode mode) { currentMode_ = mode; }
private:
  I2CTimer i2cTimer;
  char pendingBTCommand_ = 0;
  Metadata metadata_; // Add metadata storage
  IOState ioState_;
  ControlCallback controlCallback_ = nullptr;
  ButtonCallback orangeButtonCallback_ = nullptr;
  ButtonCallback bandButtonCallback_ = nullptr;
  ButtonCallback inputButtonCallback_ = nullptr;
  void parseMetadata_(const String &data);
  AudioMode currentMode_ = MODE_BLUETOOTH;
  uint8_t lastStableVolume = 0; // Last validated volume
  uint8_t pendingVolume = 0;    // Volume waiting for validation
  uint8_t stableCount = 0;      // Count of consistent readings
  NfcTagCallback nfcTagCallback_ = nullptr;
  elapsedMillis noTagTimer = 0;
  bool noTagTimerStarted = false;
  const unsigned long NO_TAG_DEBOUNCE_TIME = 3500; // 3.5 seconds
};