#include "Display.h"

#include <TimeLib.h>
#include "font/neuropolitical_10.h" // made with https://www.pjrc.com/ili9341_t3-font-editor/ and Neuropolitical font
#include "font/neuropolitical_12.h"
#include "sprites/hold_to_record.h" // TODO: Unused for now, remove or update
#include "Log.h"

DMAMEM uint16_t _fb1[320 * 240];

Display::Display(void) : tft(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCK, TFT_MISO) {};

void Display::init()
{
  tft.begin();
  tft.invertDisplay(true); // True for IPS display
  tft.setRotation(ROTATION);
  tft.useFrameBuffer(true);
  tft.setFrameBuffer(_fb1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(neuropolitical_10);
}

void Display::update()
{
  static elapsedMillis lastUpdate = 0;
  if (lastUpdate < 16)
    return; // Cap at ~60fps
  lastUpdate = 0;

  if (!splashAnimationDone)
  {
    handleSplashScreen(); // Call the new function to handle splash screen logic
  }
  tft.updateScreen();
}

void Display::updateAsync()
{
  tft.updateScreenAsync(false);
}

// Handle splash screen logic
void Display::handleSplashScreen()
{
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis() - splashStartTime;
  unsigned long updateDelta = millis() - lastUpdateTime;

  if (updateDelta >= 16)
  { // ~60fps
    lastUpdateTime = millis();
    clear();

    // Stage timings (in milliseconds)
    const unsigned long cyanStart = 2000;
    const unsigned long redStart = 200;
    const unsigned long titleStart = 2300;
    const unsigned long subtitleStart = 3300;
    const unsigned long animationEnd = 4000;

    // Alpha fade speed
    const uint8_t fadeSpeed = 16;

    // Cyan parts - simple fade in
    if (currentTime >= cyanStart)
    {
      cyanAlpha = min(255, cyanAlpha + fadeSpeed);
    }

    // Red parts - simple fade in
    if (currentTime >= redStart)
    {
      redAlpha = min(255, redAlpha + fadeSpeed);
    }

    // Title text - simple fade in
    if (currentTime >= titleStart)
    {
      textAlpha = min(255, textAlpha + fadeSpeed);
    }

    // Subtitle text - one blink
    static uint8_t subtitleAlpha = 0;
    if (currentTime >= subtitleStart)
    {
      unsigned long subtitleTime = currentTime - subtitleStart;
      if (subtitleTime < 200)
      { // Initial fade in
        subtitleAlpha = min(255, subtitleAlpha + fadeSpeed);
      }
      else if (subtitleTime < 300)
      { // Quick fade out
        subtitleAlpha = max(0, subtitleAlpha - fadeSpeed);
      }
      else
      { // Final fade in
        subtitleAlpha = min(255, subtitleAlpha + fadeSpeed);
      }
    }

    // Draw the parts
    drawSplashPart(redAlpha > 0, cyanAlpha > 0);

    // Draw main title
    if (textAlpha > 0)
    {
      uint16_t textColor = alphaBlend(ILI9341_WHITE, ILI9341_BLACK, textAlpha);
      tft.setFont(neuropolitical_12);
      tft.setTextColor(textColor);
      tft.setCursor(160, 64, true);
      tft.print("WEYLAND-YUTANI CORP");
    }

    // Draw subtitle
    if (subtitleAlpha > 0)
    {
      uint16_t subtitleColor = alphaBlend(ILI9341_WHITE, ILI9341_BLACK, subtitleAlpha);
      tft.setFont(neuropolitical_10);
      tft.setTextColor(subtitleColor);
      tft.setCursor(160, 164, true);
      tft.print("\"BUILDING BETTER WORLDS\"");
    }

    // Check if animation is complete
    if (currentTime >= animationEnd)
    {
      splashAnimationDone = true;
    }
  }
}

void Display::updateClock()
{
  String timeString = (hour() < 10 ? "0" : "") + (String)hour() + ":" + (minute() < 10 ? "0" : "") + (String)minute();
  tft.setFont(neuropolitical_12);
  tft.setTextSize(2);
  tft.fillRect(240, 20, 120, 20, ILI9341_BLACK);
  tft.setCursor(240, 20);
  tft.setTextColor(theme.clockColor);
  tft.print(timeString);
}

void Display::debugText(char *text)
{
  LOG_DISPLAY_MSG(text);
  tft.setFont(neuropolitical_10);
  tft.setTextSize(2);
  tft.fillRect(20, 0, 200, 40, ILI9341_BLACK);
  tft.setCursor(20, 20);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(text);
}

void Display::clampAndPrint(const char *text, int maxChars)
{
  if (static_cast<int>(strlen(text)) > maxChars - 1)
  {
    char truncated[maxChars];
    strncpy(truncated, text, maxChars - 4);
    truncated[maxChars - 4] = '.';
    truncated[maxChars - 3] = '.';
    truncated[maxChars - 2] = '.';
    truncated[maxChars - 1] = '\0';
    tft.print(truncated);
  }
  else
  {
    tft.print(text);
  }
}

char *normalizeUtf8Char(const char *input, char *output, bool toUpperCase)
{
  // Check first byte to determine UTF-8 sequence length
  unsigned char firstByte = *input;

  if ((firstByte & 0x80) == 0)
  { // ASCII character (0xxxxxxx)
    *output = toUpperCase ? toupper(firstByte) : firstByte;
    return output + 1;
  }

  // Get the Unicode codepoint
  uint32_t codepoint = 0;

  if ((firstByte & 0xE0) == 0xC0)
  { // 2-byte sequence (110xxxxx)
    codepoint = (firstByte & 0x1F) << 6;
    codepoint |= (input[1] & 0x3F);
  }
  else if ((firstByte & 0xF0) == 0xE0)
  { // 3-byte sequence (1110xxxx)
    codepoint = (firstByte & 0x0F) << 12;
    codepoint |= (input[1] & 0x3F) << 6;
    codepoint |= (input[2] & 0x3F);
  }
  else if ((firstByte & 0xF8) == 0xF0)
  { // 4-byte sequence (11110xxx)
    codepoint = (firstByte & 0x07) << 18;
    codepoint |= (input[1] & 0x3F) << 12;
    codepoint |= (input[2] & 0x3F) << 6;
    codepoint |= (input[3] & 0x3F);
  }

  // Map common Unicode ranges to ASCII
  char replacement;
  if (codepoint >= 0x00C0 && codepoint <= 0x00C5)
    replacement = 'A'; // À-Å
  else if (codepoint >= 0x00C8 && codepoint <= 0x00CB)
    replacement = 'E'; // È-Ë
  else if (codepoint >= 0x00CC && codepoint <= 0x00CF)
    replacement = 'I'; // Ì-Ï
  else if (codepoint >= 0x00D2 && codepoint <= 0x00D6)
    replacement = 'O'; // Ò-Ö
  else if (codepoint == 0x00D8)
    replacement = 'O'; // Ø
  else if (codepoint >= 0x00D9 && codepoint <= 0x00DC)
    replacement = 'U'; // Ù-Ü
  else if (codepoint >= 0x00DD && codepoint <= 0x00DF)
    replacement = 'Y'; // Ý-ß
  else if (codepoint >= 0x00E0 && codepoint <= 0x00E5)
    replacement = 'A'; // à-å
  else if (codepoint >= 0x00E8 && codepoint <= 0x00EB)
    replacement = 'E'; // è-ë
  else if (codepoint >= 0x00EC && codepoint <= 0x00EF)
    replacement = 'I'; // ì-ï
  else if (codepoint >= 0x00F2 && codepoint <= 0x00F6)
    replacement = 'O'; // ò-ö
  else if (codepoint == 0x00F8)
    replacement = 'O'; // ø
  else if (codepoint >= 0x00F9 && codepoint <= 0x00FC)
    replacement = 'U'; // ù-ü
  else if (codepoint >= 0x00FD && codepoint <= 0x00FF)
    replacement = 'Y'; // ý-ÿ
  else if (codepoint == 0x00C7 || codepoint == 0x00E7)
    replacement = 'C'; // Ç,ç
  else if (codepoint == 0x00D1 || codepoint == 0x00F1)
    replacement = 'N'; // Ñ,ñ
  else if (codepoint == 0x2013 || codepoint == 0x2014 || codepoint == 0x2015)
    replacement = '-'; // en-dash, em-dash, horizontal bar
  else if (codepoint == 0x2018 || codepoint == 0x2019 || codepoint == 0x201B)
    replacement = '\''; // Left/right single quotes, single reversed quote
  else if (codepoint == 0x201C || codepoint == 0x201D || codepoint == 0x201F)
    replacement = '"'; // Left/right double quotes, double reversed quote
  else if (codepoint == 0x00AB || codepoint == 0x00BB)
    replacement = '"'; // « and » (guillemets)
  else if (codepoint == 0x2039 || codepoint == 0x203A)
    replacement = '\''; // ‹ and › (single guillemets)
  else
  {
    // If no mapping found, use first byte as-is
    *output = toUpperCase ? toupper(firstByte) : firstByte;
    return output + 1;
  }

  *output = replacement;
  return output + 1;
}

void normalizeUtf8String(char *str, bool toUpperCase = false)
{
  char buffer[256]; // Temporary buffer for normalized string
  char *out = buffer;
  char *in = str;

  while (*in)
  {
    out = normalizeUtf8Char(in, out, toUpperCase);

    // Move to next UTF-8 character
    if ((*in & 0x80) == 0)
      in += 1; // ASCII
    else if ((*in & 0xE0) == 0xC0)
      in += 2; // 2-byte sequence
    else if ((*in & 0xF0) == 0xE0)
      in += 3; // 3-byte sequence
    else if ((*in & 0xF8) == 0xF0)
      in += 4; // 4-byte sequence
    else
      in += 1; // Invalid UTF-8, skip byte
  }

  *out = '\0';         // Null terminate
  strcpy(str, buffer); // Copy back to original string
}

void Display::setMetadata(const char *textBig, const char *textSmall)
{
  // If there's active temporary metadata, don't override it
  if (hasTemporaryMetadata())
  {
    return;
  }

  clearMetadataArea();
  // LOG_DISPLAY_MSG(textBig);
  // Normalize UTF-8 strings (converts to uppercase and removes accents)
  char bigBuffer[256], smallBuffer[256];
  strncpy(bigBuffer, textBig, sizeof(bigBuffer) - 1);
  strncpy(smallBuffer, textSmall, sizeof(smallBuffer) - 1);
  bigBuffer[sizeof(bigBuffer) - 1] = '\0';
  smallBuffer[sizeof(smallBuffer) - 1] = '\0';

  normalizeUtf8String(bigBuffer);
  normalizeUtf8String(smallBuffer, true);

  tft.setFont(neuropolitical_12);
  tft.setTextSize(2);
  tft.setCursor(20, 190);
  tft.setTextColor(theme.metadataLine1);
  clampAndPrint(bigBuffer, 21);

  tft.setFont(neuropolitical_10);
  tft.setTextColor(theme.metadataLine2);
  tft.setCursor(20, 210);
  clampAndPrint(smallBuffer, 28);
}

void Display::setModeTitle(const char *modeText)
{
  clear();
  LOG_DISPLAY_MSG(modeText);
  tft.setFont(neuropolitical_10);
  tft.setTextSize(2);
  // tft.fillRect(20, 20, 140, 20, ILI9341_BLACK);
  tft.setCursor(20, 20);
  tft.setTextColor(theme.modeTitle);
  tft.print(modeText);
}

void Display::drawSplash()
{
  splashStartTime = millis();
  splashAnimationDone = false;
  cyanAlpha = 0;
  redAlpha = 0;
  textAlpha = 0;
}

void Display::drawSplashPart(bool redParts, bool cyanParts)
{
  // Main red parts
  if (redAlpha > 0)
  {
    uint16_t colorW = alphaBlend(ILI9341_ORANGE, ILI9341_BLACK, redAlpha);
    tft.fillRect(19, 84, 282, 72, colorW);
    tft.fillTriangle(19, 91, 84, 156, 19, 156, ILI9341_BLACK);
    tft.fillTriangle(59, 84, 147, 84, 103, 128, ILI9341_BLACK);
    tft.fillTriangle(116, 156, 160, 112, 204, 156, ILI9341_BLACK);
    tft.fillTriangle(173, 84, 261, 84, 217, 128, ILI9341_BLACK);
    tft.fillTriangle(236, 156, 301, 91, 301, 156, ILI9341_BLACK);
  }

  // Cyan accent parts
  if (cyanAlpha > 0)
  {
    uint16_t colorY = alphaBlend(ILI9341_WHITE, ILI9341_BLACK, cyanAlpha);
    tft.fillTriangle(75, 84, 131, 84, 103, 112, colorY);
    tft.fillTriangle(189, 84, 245, 84, 217, 112, colorY);
    tft.fillRect(138, 150, 45, 6, colorY);
    tft.fillTriangle(138, 150, 160, 128, 182, 150, colorY);
  }

  // These parts always stay black
  tft.fillTriangle(119, 84, 131, 84, 125, 90, ILI9341_BLACK);
  tft.fillTriangle(189, 84, 201, 84, 195, 90, ILI9341_BLACK);
  tft.fillRect(75, 84, 6, 7, ILI9341_BLACK);
  tft.fillRect(240, 84, 6, 7, ILI9341_BLACK);
}

void Display::clearMainArea()
{
  tft.fillRect(0, 40, 320, 140, ILI9341_BLACK);
}

void Display::clearTopLeft()
{
  tft.fillRect(0, 0, 240, 40, ILI9341_BLACK);
}

void Display::clearMetadataArea()
{
  tft.fillRect(0, 180, 320, 60, ILI9341_BLACK);
}

void Display::clear()
{
  tft.fillScreen(ILI9341_BLACK);
}

void Display::drawHoldToRecord()
{
  tft.writeRect(187, 229, 135, 11, (uint16_t *)hold_to_record);
}

void Display::setTemporaryMetadata(const char *line1, const char *line2, unsigned long duration)
{
  strncpy(tempMetadataLine1, line1, sizeof(tempMetadataLine1) - 1);
  strncpy(tempMetadataLine2, line2, sizeof(tempMetadataLine2) - 1);
  tempMetadataLine1[sizeof(tempMetadataLine1) - 1] = '\0';
  tempMetadataLine2[sizeof(tempMetadataLine2) - 1] = '\0';

  // First display the metadata
  setMetadata(tempMetadataLine1, tempMetadataLine2);

  // Then set the temporary metadata state
  temporaryMetadataActive = true;
  tempMetadataTimer = 0;
  tempMetadataTimeout = duration;
}

void Display::clearTemporaryMetadata()
{
  temporaryMetadataActive = false;
}

bool Display::hasTemporaryMetadata() const
{
  return temporaryMetadataActive && (tempMetadataTimer < tempMetadataTimeout);
}