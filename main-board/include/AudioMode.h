#pragma once

enum AudioMode
{
  MODE_UNKNOWN = -1,
  MODE_TIME_SETUP = 0,
  MODE_BLUETOOTH,
  MODE_RADIO,
  MODE_SD_PLAYBACK,
  MODE_SD_RECORDER,
  MODE_NFC_PLAYBACK
};

struct AudioModeTheme {
    uint16_t modeTitle = 0xF800;
    uint16_t clockColor = 0xF800;
    uint16_t fftFront = 0xF800;
    uint16_t fftBack = 0xF800;
    uint16_t fftMain = 0xF800;
    uint16_t metadataLine1 = 0xF800;
    uint16_t metadataLine2 = 0xF800;
};
