#include "AudioModeController.h"

void AudioModeController::setDisplayTheme()
{
  display.clear();
  display.setTheme(theme);
  display.drawModeTitle(getMode());
}

void AudioModeController::setMixerGains()
{
  auto *main = audio.getMainMixer();
  main->gain(0, 0.0); // BT or radio audio
  main->gain(1, 0.0); // SD card audio L
  main->gain(2, 0.0); // SD card audio R
  main->gain(3, 0.4); // In memory audio
}

void AudioModeController::updateOutputVolume()
{
  uint8_t newVolume = isMuted ? 0 : i2c.getIOState().volume;
  audio.getOutputAmp()->gain(newVolume / 255.0);
}

void AudioModeController::configureCodec()
{
  auto *codec = audio.getCodec();
  codec->volume(0.6);
  codec->inputSelect(AUDIO_INPUT_LINEIN);
  codec->lineInLevel(8);
  codec->enhanceBassEnable();
}

AudioAnalyzePeak *AudioModeController::getPeak()
{
  return audio.getPlaybackPeak();
}