#pragma once

#include "AudioModeController.h"

class AudioModeControllerTimeSetup : public AudioModeController
{
public:
  AudioModeControllerTimeSetup(Display &display, I2C &i2c, AudioSystem &audio) : AudioModeController(display, i2c, audio) {}

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleControl(ControlCommand cmd) override {};
  void handleOrangeButton(bool pressed) override;
  AudioMode getMode() override { return MODE_TIME_SETUP; }
  bool isTimeSetComplete() { return timeSetComplete; }

private:
  enum SetupStage
  {
    YEAR,
    MONTH,
    DAY,
    HOURS,
    MINUTES
  };
  SetupStage currentStage = YEAR;
  int tempHours = 0;
  int tempMinutes = 0;
  int tempDay = 1;
  int tempMonth = 1;
  int tempYear = 2025;
  bool timeSetComplete = false;

  const char *getStageInstructions();
  int getMaxValueForStage();
  int getMinValueForStage();
};