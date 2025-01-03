#pragma once

#include "AudioModeController.h"
#include "Recorder.h"

class AudioModeControllerSDRecorder : public AudioModeController
{
private:
  Recorder &recorder;

public:
  AudioModeControllerSDRecorder(Display &display, I2C &i2c, AudioSystem &audio, Recorder &recorder)
      : AudioModeController(display, i2c, audio),
        recorder(recorder)
  {
    // https://rgbcolorpicker.com/565
    theme.modeTitle = 0xf986;
    theme.clockColor = 0xc106;
    theme.fftMain = 0xf800;
    theme.fftFront = 0xba49;
    theme.fftBack = 0xf800;
    theme.metadataLine1 = 0xf986;
    theme.metadataLine2 = 0xc106;
  }

  AudioMode getMode() override
  {
    return MODE_SD_RECORDER;
  }

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override;
  void setMixerGains() override;
  void configureCodec() override;
  void updateOutputVolume() override;
  AudioAnalyzePeak *getPeak() override;
};
