#include "AudioModeControllerSDPlayer.h"

void AudioModeControllerSDPlayer::enter()
{
  updateOutputVolume();
  setMixerGains();
  configureCodec();
  i2c.setCurrentMode(MODE_SD_PLAYBACK);
  display.setTheme(theme);
  display.setModeTitle("Player");
  recorder.setReverseAlphabeticalOrder(true);
  recorder.setBasePath("Recordings/");
  recorder.seekToFirstFile();
}

void AudioModeControllerSDPlayer::exit()
{
  recorder.stopAll();
}

void AudioModeControllerSDPlayer::loop()
{
  if (recorder.isPlaying())
  {
    recorder.continuePlaying(orangeButtonPressed);
  }
}

void AudioModeControllerSDPlayer::frameLoop()
{
  // Check for long-press delete
  if (orangeButtonPressed && orangeButtonTimer >= 3000)
  {
    orangeButtonPressed = false; // Reset to prevent multiple deletes
    if (recorder.deleteCurrentFile())
    {
      playBeep();
      display.setTemporaryMetadata((char *)"File", (char *)"Deleted", 3000);
      display.update();
      delay(2000);
      recorder.seek();
    }
  }
  else
  {
    char line1[32], line2[32];
    recorder.getMetadata(line1, line2);
    if (!display.hasTemporaryMetadata())
    {
      display.setMetadata(line1, line2);
    }
  }
}

void AudioModeControllerSDPlayer::handleOrangeButton(bool pressed)
{
  if (pressed)
  {
    orangeButtonPressed = true;
    orangeButtonTimer = 0;
    display.setTemporaryMetadata("Hold to delete", "current file", 2000);
  }
  else
  {
    display.clearTemporaryMetadata();
    orangeButtonPressed = false;
  }
}

void AudioModeControllerSDPlayer::handleControl(ControlCommand cmd)
{
  switch (cmd)
  {
  case PREV:
    recorder.playPrevFile(true);
    break;
  case PLAY:
    if (recorder.isPlaying())
    {
      recorder.stopPlaying();
    }
    else
    {
      // Try to play current file if we have one
      if (strlen(recorder.getCurrentFilename()) > 0)
      {
        recorder.playFile(recorder.getCurrentFilename());
      }
      else if (recorder.seek())
      {
        recorder.playFile(recorder.getCurrentFilename());
      }
      else
      {
        // No WAV files found - display message to user
        display.setMetadata((char *)"Nothing to play", (char *)"Go to Recorder mode");
      }
    }
    break;
  case NEXT:
    recorder.playNextFile(true);
    break;
  default:
    break;
  }
}

void AudioModeControllerSDPlayer::setMixerGains()
{
  auto *main = audio.getMainMixer();
  auto *fftInput = audio.getFFTInputMixer();

  main->gain(0, 0.0); // BT or radio audio
  main->gain(1, 0.5); // SD card audio L
  main->gain(2, 0.5); // SD card audio R
  main->gain(3, 0.3); // In memory audio

  fftInput->gain(0, 1.0); // BT/SD/Radio source
  fftInput->gain(1, 0.0); // Mic source
}

void AudioModeControllerSDPlayer::configureCodec()
{
  auto *codec = audio.getCodec();
  codec->volume(1.0);
  codec->enhanceBassDisable(); // Disable bass enhancement for SD playback
}