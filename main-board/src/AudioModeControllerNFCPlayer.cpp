#include "AudioModeControllerNFCPlayer.h"

void AudioModeControllerNFCPlayer::enter()
{
  updateOutputVolume();
  setMixerGains();
  configureCodec();
  i2c.setCurrentMode(MODE_NFC_PLAYBACK);
  display.setTheme(theme);
  display.setModeTitle("NFC Player");
  currentFolder = i2c.getIOState().nfcUidString.c_str();
  recorder.setReverseAlphabeticalOrder(false);

  String path = String(NFC_PLAYLIST_FOLDER) + "/" + currentFolder + "/";
  nfcFolderFound = SD.exists(path.c_str());

  if (nfcFolderFound)
  {
    recorder.setBasePath(path);
    recorder.seekToFirstFile();
    recorder.play();
  }
}

void AudioModeControllerNFCPlayer::exit()
{
  recorder.stopPlaying();
}

void AudioModeControllerNFCPlayer::loop()
{
  if (recorder.isPlaying())
  {
    recorder.continuePlaying();
  }
}

void AudioModeControllerNFCPlayer::frameLoop()
{
  if (!nfcFolderFound)
  {
    display.setMetadata("No playlist found", currentFolder);
    return;
  }

  char title[32], author[32];
  parseMetadataFromFilename(recorder.getCurrentFilename(), title, author);

  if (!display.hasTemporaryMetadata())
  {
    display.setMetadata(title, author);
  }
}

void AudioModeControllerNFCPlayer::parseMetadataFromFilename(const char *filename, char *title, char *author)
{
    // Handle invalid filename
    if (!filename || strlen(filename) < 5) {
        strcpy(title, "No track");
        strcpy(author, "");
        return;
    }

    // Skip track number (first 3 chars: XX_)
    const char *titleStart = strchr(filename, '_');
    if (!titleStart) {
        strncpy(title, filename, 31);
        title[31] = '\0';
        strcpy(author, "");
        return;
    }
    titleStart++; // Skip the underscore

    // Find author section
    const char *authorStart = strchr(titleStart, '_');
    if (!authorStart) {
        // No author section - copy everything up to extension as title
        strncpy(title, titleStart, 31);
        title[31] = '\0';
        if (char *ext = strstr(title, ".WAV")) *ext = '\0';
        else if (char *ext = strstr(title, ".wav")) *ext = '\0';
        strcpy(author, "");
        return;
    }

    // Copy title (between first and second underscore)
    size_t titleLen = authorStart - titleStart;
    strncpy(title, titleStart, min(titleLen, (size_t)31));
    title[min(titleLen, (size_t)31)] = '\0';

    // Copy author (after second underscore, before extension)
    authorStart++; // Skip the underscore
    strncpy(author, authorStart, 31);
    author[31] = '\0';
    if (char *ext = strstr(author, ".WAV")) *ext = '\0';
    else if (char *ext = strstr(author, ".wav")) *ext = '\0';
}

void AudioModeControllerNFCPlayer::handleOrangeButton(bool pressed)
{
}

void AudioModeControllerNFCPlayer::handleControl(ControlCommand cmd)
{
  switch (cmd)
  {
  case PREV:
    recorder.stopPlaying();
    recorder.playPrevFile(true);
    break;
  case PLAY:
    if (recorder.isPlaying())
    {
      recorder.stopPlaying();
    }
    else if (strlen(recorder.getCurrentFilename()) > 0)
    {
      recorder.play();
    }
    break;
  case NEXT:
    recorder.stopPlaying();
    recorder.playNextFile(true);
    break;
  default:
    break;
  }
}

void AudioModeControllerNFCPlayer::setMixerGains()
{
  auto *main = audio.getMainMixer();
  auto *fftInput = audio.getFFTInputMixer();

  main->gain(0, 0.0); // BT or radio audio
  main->gain(1, 0.5); // SD card audio L
  main->gain(2, 0.5); // SD card audio R
  main->gain(3, 0.0); // In memory audio

  fftInput->gain(0, 1.0); // BT/SD/Radio source
  fftInput->gain(1, 0.0); // Mic source
}

void AudioModeControllerNFCPlayer::configureCodec()
{
  auto *codec = audio.getCodec();
  codec->volume(0.6);
  codec->enhanceBassEnable();
}