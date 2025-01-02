// FM capacitor mapping
#include "FMCapacitor.h"

FMCapacitor::FMCapacitor() {}

void FMCapacitor::setup() {
  for (int thisReading = 0; thisReading < FM_NUM_READINGS; thisReading++) {
    fmCapacitorReadings_[thisReading] = 0;
  }
}

int FMCapacitor::readFMValue() {
  static unsigned long lastMeasurement = 0;
  const unsigned long MEASURE_INTERVAL = 1000; // measure every 100ms

  unsigned long currentTime = millis();
  if (currentTime - lastMeasurement < MEASURE_INTERVAL) {
    return fmCapacitorAverage_; // return last known value
  }
  lastMeasurement = currentTime;

  // Serial.println(fmCapacitor.Measure());
  // subtract the last reading:
  fmCapacitorTotal_ = fmCapacitorTotal_ - fmCapacitorReadings_[fmCapacitorReadIndex_];
  // read from the sensor:
  fmCapacitorReadings_[fmCapacitorReadIndex_] = min(max(fmCapacitor_.Measure(), FM_CAP_MIN), FM_CAP_MAX);
  // add the reading to the fmCapacitorTotal:
  fmCapacitorTotal_ = fmCapacitorTotal_ + fmCapacitorReadings_[fmCapacitorReadIndex_];
  // advance to the next position in the array:
  fmCapacitorReadIndex_ = fmCapacitorReadIndex_ + 1;

  // if we're at the end of the array...
  if (fmCapacitorReadIndex_ >= FM_NUM_READINGS) {
    // ...wrap around to the beginning:
    fmCapacitorReadIndex_ = 0;
  }

  // calculate the fmCapacitorAverage:
  fmCapacitorAverage_ = fmCapacitorTotal_ / FM_NUM_READINGS;

  // float val = 1 - ((max(min(cap1.Measure(), FM_CAP_MAX), FM_CAP_MIN) - FM_CAP_MIN) / (float)FM_CAP_MAX);
  // int fm = round(FM_BAND_MIN + val * (FM_BAND_MAX - FM_BAND_MIN));
  // int offset = fm % 100 > 50 ? 50 : 0;
  // return 100 * floor(fm / 100) + offset;
  float progressPct = 1 - (((float)fmCapacitorAverage_ - FM_CAP_MIN) / (FM_CAP_MAX - FM_CAP_MIN));
  return round(progressPct * 1023);
  // return round(progressPct * (FM_BAND_MAX - FM_BAND_MIN) + FM_BAND_MIN);
  // return fmCapacitor_.Measure();
}