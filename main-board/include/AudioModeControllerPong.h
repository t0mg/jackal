#ifndef AUDIOMODECONTROLLERPONG_H
#define AUDIOMODECONTROLLERPONG_H

#include "AudioModeController.h"
#include "Display.h"
#include "I2C.h"
#include "sound/beep.h"
#include "sound/boop.h"

class AudioModeControllerPong : public AudioModeController
{
public:
  AudioModeControllerPong(Display &display, I2C &i2c, AudioSystem &audio)
      : AudioModeController(display, i2c, audio), display(display), i2c(i2c)
  {
    // // https://rgbcolorpicker.com/565
    // theme.modeTitle = 0xfc86;
    // theme.clockColor = 0xc224;
    // theme.fftMain = 0xfda4;
    // theme.fftFront = 0xfd4d;
    // theme.fftBack = 0xf800;
    // theme.metadataLine1 = 0xfc86;
    // theme.metadataLine2 = 0xc224;

    // Initialize Pong variables
    ballX = 160;
    ballY = 120;
    ballXSpeed = 3;
    ballYSpeed = 3;
    paddle1Y = 100;
    paddle2Y = 100;
    score1 = 0;
    score2 = 0;
  }

  void enter() override;
  void exit() override;
  void loop() override;
  void frameLoop() override;
  void handleOrangeButton(bool pressed) override;
  void handleControl(ControlCommand cmd) override {};
  void updateOutputVolume() override {};
  void setMixerGains() override;
  AudioMode getMode() override { return MODE_PONG; }
  AudioAnalyzePeak *getPeak() override { return nullptr; }

private:
  Display &display;
  I2C &i2c;

  // Pong game variables
  int ballX, ballY;
  int ballXSpeed, ballYSpeed;
  int paddle1Y, paddle2Y;
  const int paddleWidth = 5;
  const int paddleHeight = 40;
  const int paddleSideOffset = 10;
  const int paddleTopOffset = 8;
  const int paddleBottomOffset = 8;
  const float ballSpeedIncreaseFactor = 1.2;
  const int initialBallXSpeed = 3;
  const int initialBallYSpeed = 3;
  const int maxBallXSpeed = 80;
  const int maxBallYSpeed = 80;
  int score1, score2;

  void draw();
  void updateGame();
  void resetBall();
  void handleCollisions();
  void increaseBallSpeed();
  void resetScore()
  {
    score1 = 0;
    score2 = 0;
  }
};

#endif // AUDIOMODECONTROLLERPONG_H
