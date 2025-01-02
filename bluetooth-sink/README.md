# Jackal - Bluetooth Sink Module

Part of the [Jackal project](../), this project implements a Bluetooth A2DP audio sink on an M5Stack Atom (ESP32), acting as both an I2C target (address `0x03`) and I2S audio source. It's designed to work in conjunction with the Teensy 4.0 [main board](../main-board) that controls it via I2C.

## Hardware

- **MCU**: M5Stack Atom (ESP32-based), note that [A2DP is not compatible with ESP32-S2, ESP32-S3, ESP32-C2 and ESP32-C3](https://github.com/pschatzmann/ESP32-A2DP/wiki/Support-for-esp32-c3%3F).

## Pinout

- **I2C**: 
  - SDA: G25
  - SCL: G21
- **I2S**:
  - LRCK: G22
  - BCK: G19
  - DATA: G23
- **LED**: G27 (Built-in RGB LED)

## Architecture

### 1. Bluetooth A2DP Sink
- Implements a Bluetooth audio sink using ESP32's A2DP profile
- Handles Bluetooth device connections and audio streaming
- Tracks metadata (title, artist) and playback state
- Device appears as "Jackal" in Bluetooth scanning
- The module implements a recovery system that monitors the Bluetooth audio stack state and reinitializes the I2S interface if necessary in an attempt to maintain stable audio output.

### 2. I2C Target Interface
The module responds at address 0x03 to I2C commands from the Teensy controller:

**Commands** (Single byte):
- `'p'` - Play
- `'s'` - Stop/Pause
- `'r'` - Previous track
- `'n'` - Next track

**Status Requests**:
Returns a 136 byte string with the format: `|T{title}|A{artist}|S{state}|C{connected_device}`
- Title and artist are truncated to 32 characters
- State can be "PLAYING", "STOPPED", or "UNKNOWN"
- Connected device shows the Bluetooth device name or "disconnected"

### 3. I2S Audio Output
- Configured for 44.1kHz sample rate
- 16-bit stereo output
- Uses APLL for accurate clock
- Attempts to auto-recover from potential audio stack issues

## LED Status Indicators

The built-in RGB LED indicates various commands being acknowledged:
- Blue: Prev/next or unknown command received
- Green: Play command received
- Red: Stop command received
- Orange: Command ignored (cooldown/debounce period)
- Yellow: During initialization
- Off: Normal operation

## Dependencies

This project could not be built without the contributions of many talented people. The bluetooth-sink has the following dependencies.

- [Phil Schatzmann's **amazing** ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP)
- [Phil Schatzmann's arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools)
- [Adafruit's NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)

## Building and Flashing

1. Install PlatformIO
2. Clone this repository
3. Open in PlatformIO IDE
4. Build and upload to the ESP32

## Debug Mode

Debug logging can be enabled by setting `DEBUG_BT_AUDIO` to `true` in `main.cpp`. This will output detailed information about Bluetooth connections, audio state, and I2C communications through the serial port at 115200 baud.