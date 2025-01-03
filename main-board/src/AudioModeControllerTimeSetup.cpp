#include <TimeLib.h>

#include "AudioModeControllerTimeSetup.h"

void AudioModeControllerTimeSetup::enter()
{
  display.clear();
  display.update();
}

void AudioModeControllerTimeSetup::exit()
{
}

void AudioModeControllerTimeSetup::loop()
{
}

const char *AudioModeControllerTimeSetup::getStageInstructions()
{
  if (timeSetComplete)
  {
    return "Time set complete";
  }
  switch (currentStage)
  {
  case YEAR:
    return "Setting clock: Year";
  case MONTH:
    return "Setting clock: Month";
  case DAY:
    return "Setting clock: Day";
  case HOURS:
    return "Setting clock: Hours";
  case MINUTES:
    return "Setting clock: Minutes";
  default:
    return "";
  }
}

int AudioModeControllerTimeSetup::getMaxValueForStage()
{
  switch (currentStage)
  {
  case HOURS:
    return 23;
  case MINUTES:
    return 59;
  case DAY:
    // Simple month length check
    if (tempMonth == 2)
      return 28;
    else if (tempMonth == 4 || tempMonth == 6 ||
             tempMonth == 9 || tempMonth == 11)
      return 30;
    else
      return 31;
  case MONTH:
    return 12;
  case YEAR:
    return 2048;
  default:
    return 0;
  }
}

int AudioModeControllerTimeSetup::getMinValueForStage()
{
  switch (currentStage)
  {
  case YEAR:
    return 2025;
  default:
    return currentStage == MONTH || currentStage == DAY ? 1 : 0;
  }
}

void AudioModeControllerTimeSetup::frameLoop()
{
  static elapsedMillis refreshTimer = 0;
  static elapsedMillis blinkTimer = 0;
  static bool blinkState = true;

  if (!timeSetComplete)
  {

    // Map tuning pot (0-255) to appropriate range for current stage
    int minVal = getMinValueForStage();
    int maxVal = getMaxValueForStage();
    int value = map(i2c.getIOState().tuning, 0, 255, minVal, maxVal);

    // Update appropriate value based on current stage
    switch (currentStage)
    {
    case HOURS:
      tempHours = value;
      break;
    case MINUTES:
      tempMinutes = value;
      break;
    case DAY:
      tempDay = value;
      break;
    case MONTH:
      tempMonth = value;
      break;
    case YEAR:
      tempYear = value;
      break;
    default:
      break;
    }
  }

  // Update blink state every 500ms
  if (blinkTimer >= 500)
  {
    blinkTimer = 0;
    blinkState = timeSetComplete ? true : !blinkState;
  }

  // Refresh display every 100ms
  if (refreshTimer >= 100)
  {
    refreshTimer = 0;
    display.clear();

    // Show title
    display.tft.setCursor(20, 40);
    display.tft.setTextColor(0xfc86);
    display.tft.print(getStageInstructions());

    // Show current date/time
    display.tft.setCursor(60, 80);
    display.tft.print("Date: ");

    // Day
    display.tft.setTextColor((blinkState || currentStage != DAY) ? 0xfc86 : 0x0000);
    display.tft.printf("%02d", tempDay);
    display.tft.setTextColor(0xfc86);
    display.tft.print("/");

    // Month
    display.tft.setTextColor((blinkState || currentStage != MONTH) ? 0xfc86 : 0x0000);
    display.tft.printf("%02d", tempMonth);
    display.tft.setTextColor(0xfc86);
    display.tft.print("/");

    // Year
    display.tft.setTextColor((blinkState || currentStage != YEAR) ? 0xfc86 : 0x0000);
    display.tft.printf("%02d", tempYear);

    // Time
    display.tft.setTextColor(0xfc86);
    display.tft.setCursor(60, 110);
    display.tft.print("Time: ");

    // Hours
    display.tft.setTextColor((blinkState || currentStage != HOURS) ? 0xfc86 : 0x0000);
    display.tft.printf("%02d", tempHours);
    display.tft.setTextColor(0xfc86);
    display.tft.print(":");

    // Minutes
    display.tft.setTextColor((blinkState || currentStage != MINUTES) ? 0xfc86 : 0x0000);
    display.tft.printf("%02d", tempMinutes);

    // Instructions

    display.tft.setCursor(20, 160);
    if (timeSetComplete)
    {
      display.tft.setTextColor(0xf800);
      display.tft.print("Flip RADIO switch to exit");
    }
    else
    {
      display.tft.setTextColor(0xc224);
      display.tft.print("Use TV TUNE to adjust");
      display.tft.setCursor(20, 180);
      display.tft.print("Press ORANGE BUTTON");
      display.tft.setCursor(20, 200);
      display.tft.print("to confirm");
    }
  }
}

void AudioModeControllerTimeSetup::handleOrangeButton(bool pressed)
{
  if (!pressed)
  {
    if (timeSetComplete) {
      playBeep();
    }
    return; // Only handle button release
  }

  switch (currentStage)
  {
  case YEAR:
    currentStage = MONTH;
    break;
  case MONTH:
    currentStage = DAY;
    break;
  case DAY:
    currentStage = HOURS;
    break;
  case HOURS:
    currentStage = MINUTES;
    break;
  case MINUTES:
    // Time setup complete
    setTime(tempHours, tempMinutes, 0, tempDay, tempMonth, tempYear);
    Teensy3Clock.set(now());
    timeSetComplete = true;
    break;
  default:
    break;
  }
}