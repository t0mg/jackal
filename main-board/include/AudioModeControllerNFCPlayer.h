#pragma once

#include "AudioModeController.h"
#include "Display.h"
#include "I2C.h"
#include "AudioSystem.h"
#include "Recorder.h"

#define NFC_PLAYLIST_FOLDER "NFC Playlists"

class AudioModeControllerNFCPlayer : public AudioModeController
{
public:
  AudioModeControllerNFCPlayer(Display &display, I2C &i2c, AudioSystem &audio, Recorder &recorder)
      : AudioModeController(display, i2c, audio), recorder(recorder)
  {
    // https://rgbcolorpicker.com/565
    // https://lospec.com/palette-list/technobike
    theme.clockColor = 0xeee7; // yellow;
    theme.fftFront = 0xc125; // green;
    theme.fftBack = 0xeee7; // yellow;
    theme.fftMain = 0x14a9; // green;
    theme.metadataLine1 = 0x2b3b; // blue;
    theme.metadataLine2 = 0xc135; // fushia;
    theme.modeTitle = 0x14a9; // green;
  }

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override;
  void setMixerGains() override;
  void configureCodec() override;
  void parseMetadataFromFilename(const char *filename, char *title, char *author);

  AudioMode getMode() override
  {
    return MODE_NFC_PLAYBACK;
  }

private:
  Recorder &recorder;
  const char *currentFolder;
  bool nfcFolderFound = false;
};