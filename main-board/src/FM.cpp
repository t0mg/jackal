#include "FM.h"
#include "Log.h"
#include "../lib/RDA5807/src/RDA5807.h" // Using Wire1 (renamed all Wire calls in RDA5807.cpp to Wire1)
#include <TimeLib.h>

void FM::init()
{
  static elapsedMillis initTimer = 0;
  static uint8_t initStep = 0;
  static bool initStarted = false;

  if (!initStarted)
  {
    LOG_FM_MSG("Starting FM radio initialization");
    initStarted = true;
    initTimer = 0;
  } else {
    LOG_FM_MSG("FM radio initialization already started");
  }

  // Array of function pointers and their delays
  static const struct
  {
    void (FM::*func)();
    int delay;
  } initSequence[] = {
      {&FM::powerDown, 100},
      {&FM::powerUp, 100},
      {&FM::softReset, 100},
      {&FM::setupRadio, 50},
      {&FM::initVolume, 10},
      {&FM::initBand, 10},
      {&FM::initMono, 10},
      {&FM::initBass, 10},
      {&FM::initRDS, 10},
      {&FM::initRdsFifo, 10},
      {&FM::initSeekThreshold, 10}};

  const int numSteps = sizeof(initSequence) / sizeof(initSequence[0]);

  if (initStep < numSteps)
  {
    if ((int)initTimer >= initSequence[initStep].delay)
    {
      LOG_FM_MSGF("Executing step %d", initStep);
      i2cTimer->startManualOperation();
      (this->*initSequence[initStep].func)();
      i2cTimer->releaseBus();
      initTimer = 0;
      initStep++;
    }
  }
  else if (initStep == numSteps)
  {
    LOG_FM_MSG("Setting initial frequency");
    i2cTimer->startManualOperation();
    if (SNVS_LPGPR0 > 8760 && SNVS_LPGPR0 < 10800)
    {
      LOG_FM_MSGF("Setting initial frequency to %u", SNVS_LPGPR0);
      rx.setFrequency(SNVS_LPGPR0);
      rx.waitAndFinishTune();
      i2cTimer->releaseBus();
    }
    rx.setFmDeemphasis(1);
    i2cTimer->releaseBus();
    LOG_FM_MSG("FM radio initialization complete");
    initComplete = true;
    initStep++;
  }
}

void FM::checkRDS()
{
  LOG_FM_MSGF("Checking RDS %d", millis());

  updateRealFrequency();

  if (!rx.hasRdsInfo())
  {
    LOG_FM_MSG("No RDS info available");
    return;
  }

  LOG_FM_MSG("RDS info available");
  char *tempRdsMsg = rx.getRdsText2A();
  char *tempStationName = rx.getRdsText0A();

  if (tempRdsMsg != nullptr)
  {
    // Validate RDS message contains only printable characters
    bool valid = true;
    for (int i = 0; tempRdsMsg[i] != '\0' && i < 64; i++) {
      if (!isprint(tempRdsMsg[i]) && !isspace(tempRdsMsg[i])) {
        valid = false;
        break;
      }
    }

    if (valid) {
      LOG_FM_MSGF("RDS message: %s", tempRdsMsg);
      tempRdsMsg[22] = '\0';
      newRDSMsg = strcmp(bufferRdsMsg, tempRdsMsg) != 0;
      if (newRDSMsg) {
        strncpy(bufferRdsMsg, tempRdsMsg, sizeof(bufferRdsMsg) - 1);
        bufferRdsMsg[sizeof(bufferRdsMsg) - 1] = '\0';
        rdsMsg = bufferRdsMsg;
      }
    }
  }

  if (tempStationName != nullptr && (millis() - stationNameElapsed) > 1000)
  {
    // Validate station name contains only printable characters
    bool valid = true;
    for (int i = 0; tempStationName[i] != '\0' && i < 16; i++) {
      if (!isprint(tempStationName[i]) && !isspace(tempStationName[i])) {
        valid = false;
        break;
      }
    }

    if (valid) {
      LOG_FM_MSGF("Station name: %s", tempStationName);
      newStationName = strncmp(bufferStatioName, tempStationName, 3) != 0;
      if (newStationName) {
        strncpy(bufferStatioName, tempStationName, sizeof(bufferStatioName) - 1);
        bufferStatioName[sizeof(bufferStatioName) - 1] = '\0';
        stationName = bufferStatioName;
      }
    }
    else
    {
      LOG_FM_MSG("No station name available");
    }
    stationNameElapsed = millis();
  }

  if ((millis() - clear_fifo) > 10000)
  {
    rx.clearRdsFifo();
    clear_fifo = millis();
  }
}

void FM::off()
{
  i2cTimer->startManualOperation();
  rx.powerDown();
  i2cTimer->releaseBus();
}

void FM::on()
{
  i2cTimer->startManualOperation();
  rx.powerUp();
  i2cTimer->releaseBus();
}

void FM::seek(bool up)
{
  i2cTimer->startManualOperation();
  rx.seek(RDA_SEEK_WRAP, up ? RDA_SEEK_UP : RDA_SEEK_DOWN, FM::nullFunc);
  currentFreq = SNVS_LPGPR0 = rx.getFrequency();
  i2cTimer->releaseBus();
}

void FM::waitSeekComplete(void)
{
  i2cTimer->startManualOperation();
  rx.waitAndFinishTune();
  i2cTimer->releaseBus();
}

void FM::resetRDSData()
{
  i2cTimer->startManualOperation();
  rx.clearRdsFifo();
  i2cTimer->releaseBus();
  newStationName = true;
  newRDSMsg = true;
  stationName = (char *)"";
  rdsMsg = (char *)"";
}

void FM::updateRealFrequency(void)
{
  uint16_t testFreq = rx.getRealFrequency();
  unsigned long currentTime = millis();

  if (testFreq < 8760 || testFreq > 10800)
  {
    LOGF("Invalid frequency %d detected, keeping previous frequency %d\n", testFreq, SNVS_LPGPR0);
    // Optionally attempt recovery
    // rx.softReset();
    // rx.setFrequency(SNVS_LPGPR0);
    currentFreq = SNVS_LPGPR0;
    return;
  }

  if (testFreq != candidateFrequency)
  {
    candidateFrequency = testFreq;
    lastFrequencyUpdate = currentTime;
  }

  if (currentTime - lastFrequencyUpdate >= FREQUENCY_DEBOUNCE_MS)
  {
    if (abs((int)(candidateFrequency - SNVS_LPGPR0)) > 1000) {
      // Suspicious jump - ignore it
      return;
    }
    lastFrequencyUpdate = currentTime;
    currentFreq = SNVS_LPGPR0 = candidateFrequency;
  }
}

void FM::setFrequency(int newFreq)
{ 
  i2cTimer->startManualOperation();
  currentFreq = newFreq;
  rx.setFrequency(newFreq);
  i2cTimer->releaseBus();
}

char *FM::getFrequencyString()
{
  char tmp[10];
  sprintf(tmp, "%5.5u", currentFreq);

  // Start index for copying - skip leading zeros
  int startIdx = 0;
  while (tmp[startIdx] == '0' && startIdx < 3) startIdx++;

  // Copy digits before decimal point
  int freqIdx = 0;
  for (int i = startIdx; i < 3; i++) {
    freq[freqIdx++] = tmp[i];
  }

  // Add decimal point and remaining digits
  freq[freqIdx++] = '.';
  freq[freqIdx++] = tmp[3];
  freq[freqIdx++] = tmp[4];
  freq[freqIdx] = '\0';

  return freq;
}

void FM::update()
{
  if (!i2cTimer->shouldPollRDS())
  {
    return;
  }
  rx.getRdsReady();
  checkRDS();

  i2cTimer->markRDSPolled();
  i2cTimer->releaseBus();
}
