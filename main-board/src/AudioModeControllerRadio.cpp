#include "AudioModeControllerRadio.h"

void AudioModeControllerRadio::enter()
{
  setMixerGains();
  configureCodec();
  updateOutputVolume();
  i2c.setCurrentMode(MODE_RADIO);
  if (!radio.isInitialized())
  {
    radio.init();
  }
  display.setTheme(theme);
  display.setModeTitle("FM Radio");
}

void AudioModeControllerRadio::exit()
{
}

void AudioModeControllerRadio::loop()
{
  if (!radio.isInitialized())
  {
    radio.init();
  }
  else
  {
    radio.update();
  }
}

void AudioModeControllerRadio::frameLoop()
{
  if (radio.newRDSMsg || radio.newStationName || SNVS_LPGPR0 != freq)
  {
    char freqDisplay[30];
    sprintf(freqDisplay, "%s Mhz %s", radio.getFrequencyString(), radio.stationName != nullptr ? radio.stationName : (char *)"");
    display.setMetadata(isMuted ? (char *)"Muted (Play to unmute)" : (radio.rdsMsg != nullptr ? radio.rdsMsg : (char *)""), freqDisplay);
  }

  // Check for favorite save
  if (orangeButtonPressed && orangeButtonTimer >= 3000)
  {
    orangeButtonPressed = false; // Reset to prevent multiple saves
    SNVS_LPGPR2 = SNVS_LPGPR0;          // Save current frequency
    display.setTemporaryMetadata("Favorite saved", "Short press to load", 3000);
    playBeep();
  }
}

void AudioModeControllerRadio::handleOrangeButton(bool pressed)
{
  if (pressed)
  {
    orangeButtonPressed = true;
    orangeButtonTimer = 0;
    display.setTemporaryMetadata("Favorite", "Hold to set", 3000);
  }
  else
  {
    display.clearTemporaryMetadata();
    if (orangeButtonTimer < 3000)
    {
      // Short press - load favorite
      uint32_t savedFreq = SNVS_LPGPR2;
      if (savedFreq >= 8760 && savedFreq <= 10800)
      {
        radio.resetRDSData();
        radio.setFrequency(savedFreq);
        radio.waitSeekComplete();
        char freqStr[12];
        sprintf(freqStr, "%s Mhz", radio.getFrequencyString());
        display.setTemporaryMetadata(freqStr, "Favorite loaded", 3000);
      }
      else
      {
        display.setTemporaryMetadata("No favorite found", "Hold to set", 3000);
      }
    }
    orangeButtonPressed = false;
  }
}

void AudioModeControllerRadio::handleControl(ControlCommand cmd)
{
  switch (cmd)
  {
  case PREV:
    radio.resetRDSData();
    radio.seek(false);
    break;
  case PLAY:
    isMuted = !isMuted;
    updateOutputVolume();
    display.setTemporaryMetadata(isMuted ? "Muted" : "Unmuted", "", 1000);
    break;
  case NEXT:
    radio.resetRDSData();
    radio.seek(true);
    break;
  default:
    break;
  }
}

void AudioModeControllerRadio::setMixerGains()
{
  auto *mono = audio.getMonoDownmixer();
  auto *main = audio.getMainMixer();
  auto *fftInput = audio.getFFTInputMixer();
  mono->gain(0, 0.0); // BT audio L
  mono->gain(1, 0.0); // BT audio R
  mono->gain(2, 0.5); // Radio audio L
  mono->gain(3, 0.5); // Radio audio R
  main->gain(0, 1.0); // Radio audio
  main->gain(1, 0.0); // SD card audio L
  main->gain(2, 0.0); // SD card audio R
  main->gain(3, 0.5); // In memory audio

  fftInput->gain(0, 1.0); // BT/SD/Radio source
  fftInput->gain(1, 0.0); // Mic source
}
