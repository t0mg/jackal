# Jackal project 

<img width="100%" alt="Jackal project banner" src="https://github.com/user-attachments/assets/f1732aa2-d4d8-46c6-8610-b3213fc993b4" />
<br>
<br>
A battery operated, multi-mode audio system built in an upcycled 1978 Sony FX-300, that supports Bluetooth audio with metadata, FM radio with RDS, SD card playback/recording (over built-in microphone), and NFC-triggered playlists.
<br>
<br>
<img width="100%" alt="Jackal project photos" src="https://github.com/user-attachments/assets/5fbd90b5-b067-4e28-ba42-1b0794df358f" />

In 2021, I upcycled a defunct 1978 Sony FX-300 — aka _Jackal_ — portable tv/radio set into a modern audio system, as detailed [in this story](https://threadreaderapp.com/thread/1465465658519572480.html). I didn't release any code at the time because it was not fully working (bluetooth was analog, FM radio didn't work, the sound quality was not good enough, the TFT display wan't great either).

I got back into it in 2024, and after gutting the thing again and redoing it almost from scratch, not forgetting to throw in 2 extra microcontrollers and a good amount of overengineering (that story is detailed [in this Bluesky thread](https://bsky.app/profile/tomgranger.bsky.social/post/3lfksgtyi4c2h) or [here](https://threadreaderapp.com/thread/1878502366451843553.html)), I'm now sharing what I consider the final version of this project. You might not want to build the exact same thing yourself, but I figured this might help inspire other projects.

This repository contains 3 subfolders as the project now uses 3 microcontrollers. The [`main-board`](./main-board) folder contains the Teensy 4.0 code. The Teensy 4.0 is the main controller, and is designed to be connected to the [bluetooth-sink](./bluetooth-sink) and the [io-board](./io-board).

Additional features include :
- Real time FFT visualization @30fps
- Real time bitcrusher effect
- Working VU meter for volume
- Favorite function in radio mode
- 5 band equalizer
- Backlight control
- Clock and settings memorization with backup coin cell battery
- MTP mode to read/write audio files on the SD card over USB
- No coding required to add a playlist for a new NFC tag (the NFC id just needs to match the name of a folder on the SD card)
- Working headphone jack (mono though, which isn't convenient for modern headphones)
- Boot animation and SFX from a movie that came out one year after the Jackal
- Dedicated setup screen for time and date when the backup battery is replaced
- And probably more useless stuff even I don't remember

## Hardware overview

- Teensy 4.0 microcontroller
  - Teensy Audio board with SGTL5000 audio codec & SD card reader
  - ILI9341 TFT display (SPI interface)
  - Acts as I2C Controller for other peripherals
  - See [main-board](./main-board/README.md) for more details

- Audio sources
  - M5Stack Atom (ESP32) as bluetooth A2DP sink
    - I2C Target for playback control, status feedback and metadata
    - I2S 16bit 44.1kHz audio source
    - See [bluetooth-sink](./bluetooth-sink/README.md) for more details
  - RDA5807 FM radio module
    - I2C Target for control and RDS data
    - Analog input for FM signal
  - Velleman M300 electret microphone (analog mic input into the audio board)
  - SD card with 16bit 44.1kHz PCM WAV files (SD card reader on the audio board)

- Arduino Nano
  - I2C Target for the Teensy (through a logic level shifter to stick to 3.3V)
  - Reads all physical buttons and potentiometers of the Jackal
  - Controls the PN532 NFC RFID reader over software serial (using I2C would add more complexity and require another logic level shifter)
  - See [io-board](./io-board/README.md) for more details

- PAM8406 amplifier (5W)
- 10000 mAh external USB battery
- Resistors, capacitors, yellow LEDs, and tons of wires.

## Dependencies

This project could not be built without the contributions of many talented people. Please refer to the dependencies section in the README of each subfolder for more details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
