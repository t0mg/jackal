#pragma once

#include "Display.h"
#include "I2C.h"
#include "AudioSystem.h"
#include "AudioMode.h"
#include "sound/beep.h"

class AudioModeController
{
protected:
  Display &display;
  I2C &i2c;
  AudioSystem &audio;
  bool isMuted = false;
  AudioModeTheme theme;

public:
  AudioModeController(Display &display, I2C &i2c, AudioSystem &audio)
      : display(display), i2c(i2c), audio(audio) {}

  virtual ~AudioModeController() = default;

  // Pure virtual methods that must be implemented
  virtual AudioMode getMode() = 0;
  virtual void enter() = 0;
  virtual void exit() = 0;
  virtual void loop() = 0;
  virtual void frameLoop() = 0;
  virtual void handleOrangeButton(bool pressed) = 0;
  virtual void handleControl(ControlCommand cmd) = 0;

  // Virtual methods with default implementations
  virtual void setMixerGains();
  virtual void updateOutputVolume();
  virtual void configureCodec();
  virtual AudioAnalyzePeak *getPeak();
  virtual AudioModeTheme& getTheme() { return theme; }
  virtual void playBeep() { audio.getMemoryPlayer()->play(beep); }
  virtual void setDisplayTheme();
};
