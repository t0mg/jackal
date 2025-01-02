#pragma once

#include "AudioModeController.h"
#include "Recorder.h"

class AudioModeControllerSDPlayer : public AudioModeController
{
private:
  Recorder &recorder;
  elapsedMillis orangeButtonTimer;
  bool orangeButtonPressed = false;

public:
  AudioModeControllerSDPlayer(Display &display, I2C &i2c, AudioSystem &audio, Recorder &recorder)
      : AudioModeController(display, i2c, audio),
        recorder(recorder)
  {
    // https://rgbcolorpicker.com/565
    // https://lospec.com/palette-list/technobike
    theme.clockColor = 0x158f; // ILI9341_BLUE;
    theme.fftFront = 0x0f6b; // ILI9341_CYAN;
    theme.fftBack = 0x2b0d; // ILI9341_BLUE;
    theme.fftMain = 0x0f6b; // ILI9341_PINK;
    theme.metadataLine1 = 0x0f6b; // ILI9341_PINK;
    theme.metadataLine2 = 0x158f; // ILI9341_PINK;
    theme.modeTitle = 0x0f6b; // ILI9341_PINK;
  }

  AudioMode getMode() override
  {
    return MODE_SD_PLAYBACK;
  }

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override;
  void setMixerGains() override;
  void configureCodec() override;
};