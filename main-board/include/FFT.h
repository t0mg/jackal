#pragma once
#include <Audio.h>

#include "Display.h"

#define FFT_HISTORY_SIZE 16
#define FFT_FRONT_COLOR ILI9341_ORANGE
#define FFT_BACK_COLOR ILI9341_RED
#define FFT_FADE_AFTER 10

/* Processes data from the AudioAnalyzeFFT1024, stores a history and draws a pretty graph */
class FFT
{
public:
  FFT(void);
  void init(AudioAnalyzeFFT1024 *fft);
  bool available();
  void drawHistory(Display *display, uint8_t tuning);
  void drawNewLevels(Display *display, uint8_t tuning);
  void updatePalette(uint16_t frontColor = FFT_FRONT_COLOR, uint16_t backColor = FFT_BACK_COLOR, uint16_t mainColor = ILI9341_WHITE);

private:
  AudioAnalyzeFFT1024 *_fft;
  // An array to hold the 32 frequency bands
  float _level[32];
  // A 2D array storing the last {FFT_HISTORY_SIZE} values for each band
  int _history[FFT_HISTORY_SIZE][32];
  uint16_t _colors[FFT_HISTORY_SIZE];
  int _curHistoryIndex;
  static const int _offsetX = 6;
  static const int _marginX = (320 - 28 * _offsetX - FFT_HISTORY_SIZE * _offsetX) / 2;
  static const int _marginY = 216;
  static const int _scaleY = 50;
  void readLevels();
  uint16_t _mainColor = ILI9341_WHITE;
};
