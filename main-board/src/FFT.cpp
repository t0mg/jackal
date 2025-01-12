#include "FFT.h"

/* Processes data from the AudioAnalyzeFFT1024, stores a history and draws a pretty graph */
FFT::FFT(void) : _fft(NULL), _curHistoryIndex(0) {};

void FFT::init(AudioAnalyzeFFT1024 *fft)
{
  _fft = fft;
  _fft->windowFunction(AudioWindowHanning1024);

  // Fill out the history with zeroes.
  for (byte i = 0; i < FFT_HISTORY_SIZE; i++)
  {
    for (byte j = 0; j < 32; j++)
    {
      _history[i][j] = 0;
    }
  }

  updatePalette(FFT_FRONT_COLOR, FFT_BACK_COLOR, ILI9341_WHITE);
}

void FFT::updatePalette(uint16_t frontColor, uint16_t backColor, uint16_t mainColor)
{
  _mainColor = mainColor;
  for (byte i = 0; i < FFT_HISTORY_SIZE; i++)
  {
    uint16_t color = Display::alphaBlend(backColor, frontColor, ((float)i / FFT_HISTORY_SIZE) * 255);
    if (i >= FFT_FADE_AFTER)
    {
      color = Display::alphaBlend(ILI9341_BLACK, color, ((float)(i - FFT_FADE_AFTER) / (FFT_HISTORY_SIZE - FFT_FADE_AFTER)) * 230);
    }
    _colors[i] = color;
  }
}

bool FFT::available()
{
  return _fft->available();
}

// void FFT::drawHistory(Display *display, uint8_t tuning)
// {
//   // Serial.println(tuning);
//   // For each entry in the history array (going oldest to newest)
//   for (byte rowIndex = FFT_HISTORY_SIZE; rowIndex > 0; rowIndex--)
//   {
//     // Determine the next index based on curHistoryIndex offset.
//     byte rowDataIndex = rowIndex + _curHistoryIndex;
//     // Wrap back to zero at the end of the history stack.
//     if (rowDataIndex >= FFT_HISTORY_SIZE)
//     {
//       rowDataIndex = rowDataIndex - FFT_HISTORY_SIZE;
//     }
//     // For each value of the current history array.
//     for (byte columnIndex = 0; columnIndex < 30; columnIndex++)
//     {
//       int startY = _history[rowDataIndex][columnIndex];
//       int endY = _history[rowDataIndex][columnIndex + 1];
//       // display->tft.drawLine(
//       //     _marginX + columnIndex * _offsetX + rowIndex * _offsetX,
//       //     _marginY - startY - rowIndex * _offsetX - (30 - columnIndex) * 2,
//       //     _marginX + columnIndex * _offsetX + _offsetX + rowIndex * _offsetX,
//       //     _marginY - endY - rowIndex * _offsetX - (29 - columnIndex) * 2,
//       //     _colors[rowIndex - 1]);

//       float magicNumber = tuning / 255.0f * 2 * 48.0f;
//       float totalWidth = 300.0f;
//       float scalingFactor = 1.0 - pow((rowIndex / magicNumber), 0.5);
//       float verticalShift = rowIndex * (magicNumber / 4 * scalingFactor) + 3;
//       float rowWidth = totalWidth * scalingFactor;
//       float rowMargin = (totalWidth - rowWidth) / 2.0f;
//       float offset = rowWidth / 29.0f;
//       int x1 = 10 + (int)(rowMargin + columnIndex * offset);
//       int y1 = 180 - verticalShift - startY * scalingFactor;
//       int x2 = 10 + (int)(rowMargin + columnIndex * offset + offset);
//       int y2 = 180 - verticalShift - endY * scalingFactor;
//       display->tft.drawLine(x1, y1, x2, y2, _colors[rowIndex - 1]);
//     }
//   }
// }

void FFT::drawHistory(Display *display, uint8_t tuning)
{
  float magicNumber = tuning / 255.0f * 2 * 48.0f;
  float totalWidth = 500.0f;

  // For each entry in the history array (going newest to oldest)
  for (int rowIndex = 0; rowIndex < FFT_HISTORY_SIZE; rowIndex++)
  {
    // Determine the next index based on curHistoryIndex offset.
    int rowDataIndex = _curHistoryIndex - rowIndex;
    // Wrap back to zero at the end of the history stack.
    if (rowDataIndex < 0)
    {
      rowDataIndex = FFT_HISTORY_SIZE + rowDataIndex;
    }

    float scalingFactor = 1.0 - pow((rowIndex / magicNumber), 0.5);
    float verticalShift = rowIndex * (magicNumber / 4 * scalingFactor) + 3;
    float rowWidth = totalWidth * scalingFactor;
    float rowMargin = (totalWidth - rowWidth) / 2.0f;
    float offset = rowWidth / 31.0f;

    // For each value of the current history array.
    for (byte columnIndex = 0; columnIndex < 31; columnIndex++)
    {
      int startY = _history[rowDataIndex][columnIndex];
      int endY = _history[rowDataIndex][columnIndex + 1];
      int x1 = -90 + (int)(rowMargin + columnIndex * offset);
      int y1 = 190 - verticalShift - startY * scalingFactor;
      int x2 = -90 + (int)(rowMargin + columnIndex * offset + offset);
      int y2 = 190 - verticalShift - endY * scalingFactor;
      if (columnIndex == 0)
      {
        display->tft.drawLine(0, 190 - verticalShift, x1, y1, _colors[FFT_HISTORY_SIZE - rowIndex - 1]);
      }
      display->tft.drawLine(x1, y1, x2, y2, _colors[FFT_HISTORY_SIZE - rowIndex - 1]);
      if (columnIndex == 30)
      {
        display->tft.drawLine(x2, y2, 320, 190 - verticalShift, _colors[FFT_HISTORY_SIZE - rowIndex - 1]);
      }
    }
  }
}

void FFT::drawNewLevels(Display *display, uint8_t tuning)
{
  readLevels();

  // _curHistoryIndex--;
  // if (_curHistoryIndex < 0)
  // {
  //   _curHistoryIndex = FFT_HISTORY_SIZE - 1;
  // }

  float magicNumber = tuning / 255.0f * 2 * 48.0f;
  float totalWidth = 500.0f;
  float scalingFactor = 1.0 - pow((15 / magicNumber), 0.5);
  float verticalShift = 15 * (magicNumber / 4 * scalingFactor) + 3;
  float rowWidth = totalWidth * scalingFactor;
  float rowMargin = (totalWidth - rowWidth) / 2.0f;
  float offset = rowWidth / 31.0f;

  for (byte columnIndex = 0; columnIndex < 31; columnIndex++)
  { // cycle through the 32 channels

    int line1 = _level[columnIndex] * _scaleY;
    if (line1 > _scaleY)
    {
      line1 = _scaleY;
    }

    int line2 = columnIndex == 30 ? 0 : _level[columnIndex + 1] * _scaleY;
    if (line2 > _scaleY)
    {
      line2 = _scaleY;
    }

    // Old function
    // display->tft.drawLine(
    //     _marginX + i * _offsetX,
    //     _marginY - line1 - (31 - i) * 2,
    //     _marginX + i * _offsetX + _offsetX,
    //     _marginY - line2 - (30 - i) * 2,
    //     FFT_FRONT_COLOR);
    // display->tft.drawLine(
    //     _marginX + i * _offsetX,
    //     1 + _marginY - line1 - (31 - i) * 2,
    //     _marginX + i * _offsetX + _offsetX,
    //     1 + _marginY - line2 - (30 - i) * 2,
    //     FFT_FRONT_COLOR);

    int startY = line1;
    int endY = line2;

    int x1 = -90 + (int)(rowMargin + columnIndex * offset);
    int y1 = 190 - verticalShift - startY * scalingFactor;
    int x2 = -90 + (int)(rowMargin + columnIndex * offset + offset);
    int y2 = 190 - verticalShift - endY * scalingFactor;
    if (columnIndex == 0)
    {
      display->tft.drawLine(0, 190 - verticalShift, x1, y1, _mainColor);
      // display->tft.drawLine(0, 191 - verticalShift, x1, 1 + y1, _mainColor);
    }
    display->tft.drawLine(x1, y1, x2, y2, _mainColor);
    // display->tft.drawLine(x1, 1 + y1, x2, 1 + y2, _mainColor);
    if (columnIndex == 30)
    {
      display->tft.drawLine(x2, y2, 320, 190 - verticalShift, _mainColor);
      // display->tft.drawLine(x2, 1 + y2, 320, 191 - verticalShift, _mainColor);
    }

    // Store the current level only
    _history[_curHistoryIndex][columnIndex] = line1;
    
    // Store the final point in position 30
    if (columnIndex == 30) {
      _history[_curHistoryIndex][31] = line2;
    }
  }

  _curHistoryIndex--;
  if (_curHistoryIndex < 0)
  {
    _curHistoryIndex = FFT_HISTORY_SIZE - 1;
  }
}

void FFT::readLevels()
{
  const float MAGIC_FACTOR = 1.5;

  _level[0] = _fft->read(0, 1) * 1.000 * MAGIC_FACTOR;
  _level[1] = _fft->read(2, 3) * 1.552 * MAGIC_FACTOR;
  _level[2] = _fft->read(4, 5) * 1.904 * MAGIC_FACTOR;
  _level[3] = _fft->read(6, 7) * 2.178 * MAGIC_FACTOR;
  _level[4] = _fft->read(8, 9) * 2.408 * MAGIC_FACTOR;
  _level[5] = _fft->read(10, 12) * 2.702 * MAGIC_FACTOR;
  _level[6] = _fft->read(13, 15) * 2.954 * MAGIC_FACTOR;
  _level[7] = _fft->read(16, 18) * 3.178 * MAGIC_FACTOR;
  _level[8] = _fft->read(19, 22) * 3.443 * MAGIC_FACTOR;
  _level[9] = _fft->read(23, 26) * 3.681 * MAGIC_FACTOR;
  _level[10] = _fft->read(27, 31) * 3.950 * MAGIC_FACTOR;
  _level[11] = _fft->read(32, 37) * 4.239 * MAGIC_FACTOR;
  _level[12] = _fft->read(38, 43) * 4.502 * MAGIC_FACTOR;
  _level[13] = _fft->read(44, 50) * 4.782 * MAGIC_FACTOR;
  _level[14] = _fft->read(51, 58) * 5.074 * MAGIC_FACTOR;
  _level[15] = _fft->read(59, 67) * 5.376 * MAGIC_FACTOR;
  _level[16] = _fft->read(68, 78) * 5.713 * MAGIC_FACTOR;
  _level[17] = _fft->read(79, 90) * 6.049 * MAGIC_FACTOR;
  _level[18] = _fft->read(91, 104) * 6.409 * MAGIC_FACTOR;
  _level[19] = _fft->read(105, 120) * 6.787 * MAGIC_FACTOR;
  _level[20] = _fft->read(121, 138) * 7.177 * MAGIC_FACTOR;
  _level[21] = _fft->read(139, 159) * 7.596 * MAGIC_FACTOR;
  _level[22] = _fft->read(160, 182) * 8.017 * MAGIC_FACTOR;
  _level[23] = _fft->read(183, 209) * 8.473 * MAGIC_FACTOR;
  _level[24] = _fft->read(210, 240) * 8.955 * MAGIC_FACTOR;
  _level[25] = _fft->read(241, 275) * 9.457 * MAGIC_FACTOR;
  _level[26] = _fft->read(276, 315) * 9.984 * MAGIC_FACTOR;
  _level[27] = _fft->read(316, 361) * 10.544 * MAGIC_FACTOR;
  _level[28] = _fft->read(362, 413) * 11.127 * MAGIC_FACTOR;
  _level[29] = _fft->read(414, 473) * 11.747 * MAGIC_FACTOR;
  _level[30] = _fft->read(474, 542) * 12.405 * MAGIC_FACTOR;
}
