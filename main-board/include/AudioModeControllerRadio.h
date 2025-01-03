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
    theme.modeTitle = 0xfc86;
    theme.clockColor = 0xc224;
    theme.fftMain = 0xfda4;
    theme.fftFront = 0xfd4d;
    theme.fftBack = 0xf800;
    theme.metadataLine1 = 0xfc86;
    theme.metadataLine2 = 0xc224;
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