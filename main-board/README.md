# Jackal main-board

Part of the [Jackal project](../), this project implements a Teensy 4.0-based controller board that controls the Jackal system and acts as I2C Controller for the other modules.

## Hardware

- **MCU**: Teensy 4.0
- **Display**: ILI9341 TFT
- **Audio**: Teensy audio board

## Pinout

| Teensy 4.0 Pin | Audio Board | TFT Screen | Bluetooth Module | I2C Controller | Other Connections | Description |
|------------|-------------|------------|------------------|-------------|-------------------|-------------|
| G |  | GND |  |  |  | Ground |
| 0 |  | CS |  |  |  | Chip Select |
| 1 |  | MISO |  |  |  | Using SPI port 1 |
| 2 |  |  |  |  | Backlight LED | PWM Output |
| 3 |  |  | I2S LRCK |  |  | Left/Right Clock |
| 4 |  |  | I2S BCK |  |  | Bit Clock |
| 5 |  |  | DATA |  |  | Data Line |
| 6 | CS |  |  |  |  | Chip Select (memory) |
| 7 | DIN |  |  |  |  | Audio Input |
| 8 | DOUT |  |  |  |  | Audio Output |
| 9 |  | D/C |  |  |  | Data/Command |
| 10 | SDCS |  |  |  |  | Chip Select (SD card) |
| 11 | MOSI |  |  |  |  | Using SPI port 0 |
| 12 | MISO |  |  |  |  | Using SPI port 0 |
| 13 | SCK |  |  |  |  | Using SPI port 0 |
| 14 |  |  |  |  | VU Meter | PWM Output |
| 15 | Vol pot. |  |  |  |  | Volume Potentiometer |
| 16 |  |  |  | SCL |  | Clock Line, with 4.7k pull-up to 3v3 |
| 17 |  |  |  | SDA |  | Data Line, with 4.7k pull-up to 3v3 |
| 18 | SDA |  |  |  |  | Audio Control Data |
| 19 | SCL |  |  |  |  | Audio Control Clock |
| 20 | LRCLK |  |  |  |  | Audio Left/Right Clock |
| 21 | BCLK |  |  |  |  | Audio Bit Clock |
| _22_ |  |  |  |  |  | _Unused_ |
| 23 | MCLK |  |  |  |  | Audio Master Clock |
| 3V |  | RST |  |  |  | Reset |
| G | GND | GND |  |  |  | Ground |
| 5V |  | VCC |  |  |  | 5V Power |
| _24_ |  |  |  |  |  | _Unused_ |
| _25_ |  |  |  |  |  | _Unused_ |
| 26 |  | MOSI |  |  |  | Using SPI port 1 |
| 27 |  | SCK |  |  |  | Using SPI port 1 |
| _28 - 39_ |  |  |  |  |  | _Unused_ |

The Micro USB port is used for powering the Teensy 4.0 from a battery (or computer when programming / debugging).

### Notes

- For the pinout of the other microcontrollers in the project, see [io-board README](../io-board/README.md), and [bluetooth-sink README](../bluetooth-sink/README.md)
- All I2C lines (SDA/SCL) require 4.7k Ω pull-up resistors to 3.3V
- The Bluetooth module uses a separate I2S interface as well as I2C as target
- Multiple SPI devices (TFT, SD card, memory) share the same bus with different CS lines

## Architecture

### Core Components

#### Audio System (`AudioSystem`)
Manages the audio processing pipeline using the Teensy Audio Library. Handles:
- Audio mixing
- FFT analysis
- Volume control
- EQ bands
- Audio routing between different inputs/outputs

#### Display (`Display`)
Controls the ILI9341 TFT display to show:
- Current mode
- Metadata (song info, radio station, etc)
- Time
- Status information
- FFT visualization is handled separately (in `FFT`)

#### I2C Communication (`I2C`)
Handles communication with:
- IO expansion board (buttons, encoders)
- Bluetooth module (metadata, controls)
- FM radio module
- Other I2C peripherals

### Audio Modes

The system implements different audio modes through a set of controller classes that inherit from `AudioModeController`, each with its own color scheme for the display:

1. **Bluetooth Mode** (`AudioModeControllerBluetooth`)
   - Streams audio from Bluetooth devices
   - Displays device name and track metadata
   - Supports EQ adjustment

2. **FM Radio Mode** (`AudioModeControllerRadio`)
   - Controls the RDA5807 FM tuner
   - Displays frequency and RDS information
   - Supports station seeking and favoriting

3. **SD Playback Mode** (`AudioModeControllerSDPlayer`)
   - Plays WAV files from SD card
   - Shows file metadata (recording time and date)
   - Supports file navigation and deletion

4. **SD Recording Mode** (`AudioModeControllerSDRecorder`)
   - Records audio to WAV files
   - Shows recording levels
   - Manages file creation/naming

5. **NFC Playlist Mode** (`AudioModeControllerNFCPlayer`)
   - Triggers playlists based on NFC tags
   - Manages playlist browsing
   - Shows track info

### Development

This project uses PlatformIO for development. Key files:
- `platformio.ini`: Project configuration
- `src/main.cpp`: Main program logic
- `include/`: Header files
- `lib/`: Custom libraries and dependencies

## Building and Flashing

1. Install PlatformIO
2. Clone this repository
3. Open in PlatformIO IDE
4. Build and upload to Teensy 4.0

## Dependencies

This project could not be built without the contributions of many talented people. The main-board has the following dependencies.

PlatformIO loads the following in libdeps:
- [alex6679's teensy-4-async-inputs](https://github.com/alex6679/teensy-4-async-inputs)
- [KurtE's MTP_Teensy](https://github.com/KurtE/MTP_Teensy)
- [KurtE's ILI9341_t3n](https://github.com/KurtE/ILI9341_t3n)
- [Paul Stoffregen's Wire](https://github.com/PaulStoffregen/Wire)
- [Paul Stoffregen's SPI](https://github.com/PaulStoffregen/SPI)
- [Paul Stoffregen's Time](https://github.com/PaulStoffregen/Time)

The platformio.ini runs a [script](cleanup_libdeps.py) to remove some example directories from the libdeps folder that otherwise break the build.

Additionally, a couple of libraries are copied directly in the lib:
- [Ricardo Lima Caratti's RDA5807](https://github.com/pu2clr/RDA5807)
- Some classes from [JayShoe's esp32_T4_bt_music_receiver](https://github.com/JayShoe/esp32_T4_bt_music_receiver)
