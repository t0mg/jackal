#ifndef FM_H
#define FM_H

#include "../lib/RDA5807/src/RDA5807.h"
#include "Log.h"
#include "I2CTimer.h"

class FM {
private:
    RDA5807 rx;
    char* rdsTime;
    char bufferStatioName[16];
    char bufferRdsMsg[40];
    char bufferRdsTime[20];
    int currentFreq;
    char freq[10];
    long stationNameElapsed;
    long clear_fifo;
    long polling_rds;
    bool initComplete = false;
    static const uint8_t INIT_STEPS = 12;  // 11 init functions + final frequency setting
    I2CTimer* i2cTimer;
    uint16_t candidateFrequency = 0;
    unsigned long lastFrequencyUpdate = 0;
    static const unsigned long FREQUENCY_DEBOUNCE_MS = 500;

    void checkRDS();
    void powerDown() { LOG_FM_MSG("Powering down"); rx.powerDown(); }
    void powerUp() { LOG_FM_MSG("Powering up"); rx.powerUp(); }
    void softReset() { LOG_FM_MSG("Performing soft reset"); rx.softReset(); }
    void setupRadio() { LOG_FM_MSG("Setting up radio"); rx.setup(); }
    void initVolume() { LOG_FM_MSG("Setting volume"); rx.setVolume(8); }
    void initBand() { LOG_FM_MSG("Setting band"); rx.setBand(RDA_FM_BAND_USA_EU); }
    void initMono() { LOG_FM_MSG("Setting mono"); rx.setMono(true); }
    void initBass() { LOG_FM_MSG("Setting bass"); rx.setBass(true); }
    void initRDS() { LOG_FM_MSG("Setting RDS"); rx.setRDS(true); }
    void initRdsFifo() { LOG_FM_MSG("Setting RDS FIFO"); rx.setRdsFifo(true); }
    void initSeekThreshold() { LOG_FM_MSG("Setting seek threshold"); rx.setSeekThreshold(50); }
    void updateRealFrequency();

public:
    FM(I2CTimer* timer) : i2cTimer(timer) {}
    
    char* rdsMsg;
    char* stationName;
    bool newRDSMsg = false;
    bool newStationName = false;

    void init();
    void off();
    void on();
    void seek(bool up = true);
    void waitSeekComplete(void);
    void resetRDSData();
    void setFrequency(int newFreq);
    char* getFrequencyString();
    void update();
    bool isInitialized() { return initComplete; }

    static void nullFunc() {}
};

#endif // FM_H 