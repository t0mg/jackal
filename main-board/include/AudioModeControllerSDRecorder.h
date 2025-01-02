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
