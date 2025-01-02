# Jackal - I/O Board

Part of the [Jackal project](../), this Arduino Nano-based board serves as an I2C target device (address `0x02`) that interfaces with various physical controls and sensors, communicating with the Teensy 4.0 [main board](../main-board) controller.

## Hardware

- **MCU**: Arduino Nano (ATmega328P)
- **PN532**: PN532 NFC RFID module from elechouse (dipswitches set to 0/0 for HSU mode)
- **Level Shifter**: 5V to 3.3V bidirectional logic level shifter

## Pinout

| Arduino Nano Pin | Control Inputs | I2C Bus | NFC Module | Description |
|-----------------|----------------|----------|------------|-------------|
| A0 | VHF pot |  |  | VHF/TV Tuning potentiometer analog input |
| A1 | VOL pot |  |  | Volume potentiometer analog input |
| A2 | Tone pot |  |  | Tone control potentiometer analog input |
| A3 | Play/prev/next |  |  | Multi-button control panel (2.2k pull-down) |
| A4 (SDA) |  | SDA |  | I2C Data line to Teensy 4.0, through 5V to 3.3V level shifter |
| A5 (SCL) |  | SCL |  | I2C Clock line to Teensy 4.0, through 5V to 3.3V level shifter |
| A6 | Brightness pot |  |  | Display brightness control analog input |
| A7 | FM Capacitor |  |  | FM tuning capacitor analog reading |
| D2 | FM Capacitor |  |  | FM tuning capacitor digital control |
| D3 | Band select |  |  | Band selection button (pull-up enabled) |
| D4 | Orange button |  |  | Push button input (pull-up enabled) |
| D8 |  |  | TX | Software Serial TX to PN532 NFC module RXD pin (labeled SCL) |
| D9 |  |  | RX | Software Serial RX from PN532 NFC module TXD pin (labeled SDA) |
| D12 | Input select |  |  | Input selection button |
| 5V |  | VCC |  | 5V provided by Teensy 4.0 main board |
| GND |  | GND |  | Ground connection |

### Notes

- All buttons use internal pull-up resistors except for the Play/Prev/Next which uses a 2.2k pull-down resistor (all 3 buttons are using different resistors values to determine the correct button pressed)
- I2C lines (SDA/SCL) are used for communication with the Teensy 4.0 main board through a level shifter
- Software Serial is used for the PN532 NFC module to avoid conflicts with I2C communication
- The board is powered by the 5V output from the Teensy 4.0 main board

## Architecture

### 1. I2C Interface

This board operates as an I2C target device at address 0x02, responding to requests from the Teensy 4.1 main board. On each request, it sends a data packet containing:

1. Button states (1 byte)
2. Volume pot value (1 byte, mapped 0-255)
3. Tone pot value (1 byte, mapped 0-255)
4. TV tuning pot value (1 byte, mapped 0-255)
5. Brightness pot value (1 byte, mapped 0-255)
6. FM capacitor value (1 byte, mapped 0-255) - _Currently not operational, always sends 0_
7. Control command value (1 byte)
8. NFC UID (7 bytes)

### 2. Input Processing

- Potentiometer readings use moving averages to smooth values
- Button inputs are debounced using the Bounce2 library
- NFC polling occurs every 1000ms
- Analog inputs are read every 20ms

## Dependencies

This project could not be built without the contributions of many talented people. The io-board has the following dependencies.

- [Thomas O Fredericks' Bounce2](https://github.com/thomasfredericks/Bounce2)
- [Jack Christensen's movingAvg](https://github.com/JChristensen/movingAvg)
- [Don Coleman's NDEF Library](https://github.com/don/NDEF)
- [Seeed-Studio's PN532 NFC Library](https://github.com/Seeed-Studio/PN532)

## Building and Flashing

1. Install PlatformIO
2. Clone this repository
3. Open in PlatformIO IDE
4. Build and upload to Arduino Nano

### Development

- Debug output can be enabled by uncommenting `#define DEBUG`

## FM Capacitor

- FM Capacitor functionality (reading a value from the Jackal's original FM tuner) was removed as it was causing too much noise.
- The code is still there as I might revisit it in the future. It can be re-enabled with `#define ENABLE_FM_CAPACITOR` as well as adding `codewrite/Capacitor` to the `lib_deps` section in `platformio.ini`.

