
#pragma once

#include <Capacitor.h>

#define FM_IN_APIN A7
#define FM_OUT_DPIN 2

#define FM_CAP_MIN 14 //1390
#define FM_CAP_MAX 58 //5730
#define FM_BAND_MIN 7500
#define FM_BAND_MAX 9000 //10800
#define FM_NUM_READINGS 32

class FMCapacitor
{
  public:
    FMCapacitor();

    void setup();
    int readFMValue();

  private:
    // Capacitor under test.
    // Note that for electrolytics the first pin
    // should be positive, the second negative.
    Capacitor fmCapacitor_ = Capacitor(FM_OUT_DPIN, FM_IN_APIN);
    int fmCapacitorReadings_[FM_NUM_READINGS];  // the readings from the analog input
    int fmCapacitorReadIndex_ = 0;              // the index of the current reading
    int fmCapacitorTotal_ = 0;                  // the running total
    int fmCapacitorAverage_ = 0;                // the average
};