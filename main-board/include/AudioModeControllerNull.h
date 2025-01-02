#pragma once

#include "AudioModeController.h"

class AudioModeControllerNull : public AudioModeController
{
public:
  AudioModeControllerNull(AudioSystem &audioSystem) : AudioModeController(Display::null(), I2C::null(), audioSystem) {}
  AudioMode getMode() override { return MODE_UNKNOWN; }
  void enter() override {}
  void exit() override {}
  void loop() override {}
  void frameLoop() override {}
  void handleOrangeButton(bool pressed) override {}
  void handleControl(ControlCommand cmd) override {}
  void updateOutputVolume() override {}
  void setMixerGains() override {}
};