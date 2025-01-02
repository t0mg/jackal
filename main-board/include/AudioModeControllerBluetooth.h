#include "AudioModeController.h"

class AudioModeControllerBluetooth : public AudioModeController
{
private:
  bool isEqMode = false;
  int bandIndex = 0;
  elapsedMillis orangeButtonTimer;
  bool orangeButtonPressed = false;

public:
  AudioModeControllerBluetooth(Display &display, I2C &i2c, AudioSystem &audio)
      : AudioModeController(display, i2c, audio)
  {
    // https://rgbcolorpicker.com/565
    // https://lospec.com/palette-list/cyberpunk-neons
    theme.clockColor = 0x14b2;
    theme.fftFront = 0x14b2;
    theme.fftBack = 0x0a8c;
    theme.fftMain = 0x575c;
    theme.metadataLine1 = 0x575c;
    theme.metadataLine2 = 0x14b2;
    theme.modeTitle = 0x575c;
  }
  AudioMode getMode() override
  {
    return MODE_BLUETOOTH;
  }
  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override;
  void setMixerGains() override;
  void configureCodec() override;
};