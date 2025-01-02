#ifndef I2C_TIMER_H
#define I2C_TIMER_H

#include <Arduino.h>

class I2CTimer {
public:
    static const unsigned long IO_POLL_INTERVAL = 100;    // Increased from 50ms
    static const unsigned long BT_STATUS_INTERVAL = 450;  // Increased from 100ms
    static const unsigned long RETRY_INTERVAL = 50;       // Time between retries in ms
    static const uint8_t MAX_RETRIES = 0;                 // 0 retries for now
    static const unsigned long TIMEOUT = 5000;            // Increased timeout if needed
    static const unsigned long WARMUP_PERIOD = 5000;      // Time to wait for IO measurements to stabilize
    static const unsigned long RDS_POLL_INTERVAL = 300;   // RDS should be polled frequently

    // Add priority levels
    enum BusOperation {
        NONE = 0,
        MANUAL = 1,
        RDS_POLL = 2,
        BT_STATUS = 3,
        IO_POLL = 4
    };

    I2CTimer();
    void begin();
    bool isWarmingUp();
    bool shouldPollIO();
    bool shouldPollBluetooth();
    bool shouldPollRDS();
    bool shouldRetry();
    bool hasTimeout();
    void resetTimeout();
    void update();
    void markIOPolled();
    void markBTPolled();
    void markRDSPolled();
    void startRetrySequence();
    void markRetryComplete();
    bool isInRetrySequence() { return isRetrying; }
    uint8_t currentRetryCount;
    void startManualOperation() { currentOperation = MANUAL; }
    BusOperation getCurrentOperation() { return currentOperation; }
    void releaseBus() { currentOperation = NONE; }

private:
    unsigned long firstIOPoll;
    unsigned long lastIOPoll;
    unsigned long lastBTPoll;
    unsigned long lastRDSPoll;
    unsigned long lastResponse;
    unsigned long lastRetry;
    bool timeoutFlag;
    bool isRetrying;
    BusOperation currentOperation;
    unsigned long operationStartTime;
    static const unsigned long OPERATION_TIMEOUT = 200; // ms
}; 

#endif // I2C_TIMER_H