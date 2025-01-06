#include "AudioModeControllerBluetooth.h"

void AudioModeControllerBluetooth::enter()
{
  setMixerGains();
  configureCodec();
  updateOutputVolume();
  setDisplayTheme();
  i2c.setCurrentMode(MODE_BLUETOOTH);
}

void AudioModeControllerBluetooth::exit()
{
  i2c.btPause();
}

void AudioModeControllerBluetooth::loop()
{
}

void AudioModeControllerBluetooth::frameLoop()
{
  display.drawBtIcon(i2c.getMetadata().isConnected);

  if (i2c.getMetadata().isPlaying && !display.hasTemporaryMetadata())
  {
    display.setMetadata(
        (char *)i2c.getMetadata().title.c_str(),
        (char *)i2c.getMetadata().artist.c_str());
  }

  if (isEqMode)
  {
    String toneLabel = "Band " + String(bandIndex);
    display.setMetadata(
        (char *)toneLabel.c_str(),
        (char *)String(audio.getBandValue(bandIndex)).c_str());
  }
}

void AudioModeControllerBluetooth::handleOrangeButton(bool pressed)
{
  if (pressed)
  {
    orangeButtonPressed = true;
    orangeButtonTimer = 0;
    if (isEqMode)
    {
      bandIndex = (bandIndex + 1) % 5;
    }
    else
    {
      display.setTemporaryMetadata("Equalizer options", "Hold to enter", 1000);
    }
  }
  else
  {
    if (orangeButtonPressed && orangeButtonTimer >= 3000)
    {
      isEqMode = !isEqMode;
    }
    orangeButtonPressed = false;
  }
}

void AudioModeControllerBluetooth::handleControl(ControlCommand cmd)
{
  switch (cmd)
  {
  case PREV:
    if (isEqMode)
    {
      float currentValue = audio.getBandValue(bandIndex);
      audio.setBandValue(bandIndex, currentValue - 0.1f);
    }
    else
      i2c.btPrevious();
    break;
  case PLAY:
    if (i2c.getMetadata().isPlaying)
      i2c.btPause();
    else
      i2c.btPlay();
    break;
  case NEXT:
    if (isEqMode)
    {
      float currentValue = audio.getBandValue(bandIndex);
      audio.setBandValue(bandIndex, currentValue + 0.1f);
    }
    else
      i2c.btNext();
    break;
  default:
    break;
  }
}

void AudioModeControllerBluetooth::setMixerGains()
{
  auto *mono = audio.getMonoDownmixer();
  auto *main = audio.getMainMixer();
  auto *fftInput = audio.getFFTInputMixer();

  mono->gain(0, 0.9); // BT audio L
  mono->gain(1, 0.1); // BT audio R
  mono->gain(2, 0.0); // Radio audio L
  mono->gain(3, 0.0); // Radio audio R
  main->gain(0, 1.0); // BT audio
  main->gain(1, 0.0); // SD card audio L
  main->gain(2, 0.0); // SD card audio R
  main->gain(3, 0.6); // In memory audio

  fftInput->gain(0, 1.0); // BT/SD/Radio source
  fftInput->gain(1, 0.0); // Mic source
}

void AudioModeControllerBluetooth::configureCodec()
{
  auto *codec = audio.getCodec();
  codec->volume(0.6);
  codec->enhanceBassEnable();
}