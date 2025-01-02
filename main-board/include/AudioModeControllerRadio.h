#pragma once

#include "AudioModeController.h"
#include "FM.h"

class AudioModeControllerRadio : public AudioModeController
{
private:
  FM &radio;
  elapsedMillis orangeButtonTimer;
  bool orangeButtonPressed = false;
  unsigned int freq;

public:
  AudioModeControllerRadio(Display &display, I2C &i2c, AudioSystem &audio, FM &radio)
      : AudioModeController(display, i2c, audio),
        radio(radio),
        freq(SNVS_LPGPR0) // Load last frequency from NVRAM
  {
    // https://rgbcolorpicker.com/565
    // https://lospec.com/palette-list/cyberpunk-neons
    theme.clockColor = 0xd9cd;
    theme.fftFront = 0xd9cd;
    theme.fftBack = 0x0a8c;
    theme.fftMain = 0x575c;
    theme.metadataLine1 = 0x575c;
    theme.metadataLine2 = 0xd9cd;
    theme.modeTitle = 0x575c;
  }

  AudioMode getMode() override
  {
    return MODE_RADIO;
  }

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override;
  void setMixerGains() override;
};