#pragma once
#include <ILI9341_t3n.h>
#include "AudioMode.h"

#define TFT_RST 255
#define TFT_DC 9 // Was 3 on the original build
#define TFT_CS 0
#define TFT_MOSI 26
#define TFT_MISO 1
#define TFT_SCK 27

#define ROTATION 3

class Display
{
public:
  static Display &null()
  {
    static Display instance;
    return instance;
  }
  Display(void);

  ILI9341_t3n tft;

  void init();
  void update();
  void updateAsync();
  void updateClock();
  void drawModeTitle(AudioMode mode);
  void drawRecIcon(bool recording);
  void drawBtIcon(bool connected);
  void debugText(char *msg);
  void clampAndPrint(const char *text, int maxChars);
  void setMetadata(const char *textBig, const char *textSmall);
  void drawSplash();
  void clearTopLeft();
  void clearMainArea();
  void clearMetadataArea();
  void clear();
  void setTemporaryMetadata(const char *line1, const char *line2, unsigned long duration);
  void clearTemporaryMetadata();
  bool hasTemporaryMetadata() const;
  void setTheme(AudioModeTheme theme)
  {
    this->theme = theme;
  }

  /**
   * From ILI9341_t3n (in which this method is protected)
   * alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
   * @param	fg		Color to draw in RGB565 (16bit)
   * @param	bg		Color to draw over in RGB565 (16bit)
   * @param	alpha	Alpha in range 0-255
   **/
  static uint16_t alphaBlend(uint32_t fg, uint32_t bg, uint8_t alpha)
  {
    alpha = (alpha + 4) >> 3; // from 0-255 to 0-31
    bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
    fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;
    uint32_t result = ((((fg - bg) * alpha) >> 5) + bg) & 0b00000111111000001111100000011111;
    return (uint16_t)((result >> 16) | result); // contract result
  }

private:
  void handleSplashScreen();
  void drawSplashPart(bool redParts, bool cyanParts);
  unsigned long splashStartTime;
  bool splashAnimationDone;
  uint8_t cyanAlpha = 0;
  uint8_t redAlpha = 0;
  uint8_t textAlpha = 0;
  int cyanBlinks = 0;
  int redBlinks = 0;
  int textBlinks = 0;
  bool needsUpdate = false;
  char tempMetadataLine1[32];
  char tempMetadataLine2[32];
  bool temporaryMetadataActive;
  elapsedMillis tempMetadataTimer;
  unsigned long tempMetadataTimeout;
  AudioModeTheme theme;
};