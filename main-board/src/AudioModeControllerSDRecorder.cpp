#include "AudioModeControllerSDRecorder.h"

void AudioModeControllerSDRecorder::enter()
{
  setMixerGains();
  configureCodec();
  setDisplayTheme();
  i2c.setCurrentMode(MODE_SD_RECORDER);
  updateOutputVolume(); // This will mute output in recorder mode
  recorder.setReverseAlphabeticalOrder(true);
  recorder.setBasePath("Recordings/");
}

void AudioModeControllerSDRecorder::exit()
{
  if (recorder.isRecording())
  {
    recorder.stopRecording();
  }
}

void AudioModeControllerSDRecorder::loop()
{
  if (recorder.isRecording())
  {
    recorder.continueRecording();
    yield();
  }
}

void AudioModeControllerSDRecorder::frameLoop()
{
  char line1[32], line2[32];
  if (recorder.isRecording()) {
    recorder.getMetadata(line1, line2);
  } else {
    strcpy(line1, "Hold ORANGE BUTTON");
    strcpy(line2, "to start a new recording");
  }
  display.setMetadata(line1, line2);
  display.drawRecIcon(recorder.isRecording());
}

void AudioModeControllerSDRecorder::handleOrangeButton(bool pressed)
{
  if (pressed)
  {
    recorder.startRecording();
  }
  else
  {
    playBeep();
    recorder.stopRecording();
    display.setTemporaryMetadata("Recording stopped", "Saved to memory", 3000);
  }
}

void AudioModeControllerSDRecorder::handleControl(ControlCommand cmd)
{
  // Recorder mode doesn't handle any control commands
  // All controls are handled through the orange button
}

void AudioModeControllerSDRecorder::setMixerGains()
{
  auto *main = audio.getMainMixer();
  auto *fftInput = audio.getFFTInputMixer();

  main->gain(0, 0.0); // BT or radio audio
  main->gain(1, 0.0); // SD card audio L
  main->gain(2, 0.0); // SD card audio R
  main->gain(3, 0.4); // In memory audio

  fftInput->gain(0, 0.0); // BT/SD/Radio source
  fftInput->gain(1, 1.0); // Mic source
}

void AudioModeControllerSDRecorder::configureCodec()
{
  auto *codec = audio.getCodec();
  codec->volume(0.8);
  codec->inputSelect(AUDIO_INPUT_MIC);
  codec->micGain(20);
  codec->enhanceBassDisable();
}

// void AudioModeControllerSDRecorder::updateOutputVolume()
// {
//   // Always mute output in recorder mode to prevent feedback
//   audio.getOutputAmp()->gain(0);
// }

AudioAnalyzePeak *AudioModeControllerSDRecorder::getPeak()
{
  return audio.getRecorderPeak();
}