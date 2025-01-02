#pragma once
#include <Arduino.h>

// Set to 1 to enable logging, 0 to disable
#define ENABLE_LOGGING 1

// Logging categories - can be individually enabled/disabled
#define LOG_I2C        1
#define LOG_BLUETOOTH  0
#define LOG_RECORDER   1
#define LOG_DISPLAY    0
#define LOG_AUDIO      0
#define LOG_FM         0

#if ENABLE_LOGGING
    #define LOG(msg) Serial.println(msg)
    #define LOGF(...) Serial.printf(__VA_ARGS__)
    
    #if LOG_I2C
        #define LOG_I2C_MSG(msg) Serial.println(msg)
        #define LOG_I2C_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_I2C_MSG(msg)
        #define LOG_I2C_MSGF(...)
    #endif

    #if LOG_BLUETOOTH
        #define LOG_BT_MSG(msg) Serial.println(msg)
        #define LOG_BT_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_BT_MSG(msg)
        #define LOG_BT_MSGF(...)
    #endif

    #if LOG_RECORDER
        #define LOG_RECORDER_MSG(msg) Serial.println(msg)
        #define LOG_RECORDER_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_RECORDER_MSG(msg)
        #define LOG_RECORDER_MSGF(...)
    #endif

    #if LOG_DISPLAY
        #define LOG_DISPLAY_MSG(msg) Serial.println(msg)
        #define LOG_DISPLAY_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_DISPLAY_MSG(msg)
        #define LOG_DISPLAY_MSGF(...)
    #endif

    #if LOG_AUDIO
        #define LOG_AUDIO_MSG(msg) Serial.println(msg)
        #define LOG_AUDIO_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_AUDIO_MSG(msg)
        #define LOG_AUDIO_MSGF(...)
    #endif

    #if LOG_FM
        #define LOG_FM_MSG(msg) Serial.println(msg)
        #define LOG_FM_MSGF(...) Serial.printf(__VA_ARGS__)
    #else
        #define LOG_FM_MSG(msg)
        #define LOG_FM_MSGF(...)
    #endif
#else
    #define LOG(msg)
    #define LOGF(...)
    #define LOG_I2C_MSG(msg)
    #define LOG_I2C_MSGF(...)
    #define LOG_BT_MSG(msg)
    #define LOG_BT_MSGF(...)
    #define LOG_DISPLAY_MSG(msg)
    #define LOG_DISPLAY_MSGF(...)
    #define LOG_AUDIO_MSG(msg)
    #define LOG_AUDIO_MSGF(...)
#endif