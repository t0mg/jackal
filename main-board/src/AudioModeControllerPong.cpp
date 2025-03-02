#include "AudioModeControllerPong.h"
#include "Log.h"
#include <cmath>

#define PIN_VUMETER 14

void AudioModeControllerPong::enter()
{
  LOG("Entering Pong mode");
  audio.getOutputAmp()->gain(0.1);
  audio.getBitcrusher()->bits(8);
  audio.getBitcrusher()->sampleRate(11025);
  i2c.getTimer().setFastIO(true);
  resetBall();
}

void AudioModeControllerPong::exit()
{
  LOG("Exiting Pong mode");
  i2c.getTimer().setFastIO(false);
}

void AudioModeControllerPong::setMixerGains()
{
  auto *main = audio.getMainMixer();
  main->gain(0, 0.0); // BT or radio audio
  main->gain(1, 0.0); // SD card audio L
  main->gain(2, 0.0); // SD card audio R
  main->gain(3, 0.4); // In memory audio
}

void AudioModeControllerPong::handleOrangeButton(bool pressed)
{
  if (pressed)
  {
    resetScore();
    resetBall();
  }
}

void AudioModeControllerPong::loop() {}

void AudioModeControllerPong::frameLoop()
{
  updateGame();
  analogWrite(PIN_VUMETER, round(ballX / 32));
  draw();
}

void AudioModeControllerPong::draw()
{
  display.clear();

  // Offset the paddles by 20 pixels from the side edges and 5 from top and bottom
  int paddle1X = paddleSideOffset;
  int paddle2X = 320 - paddleWidth - paddleSideOffset;

  display.tft.fillRect(paddle1X, paddle1Y, paddleWidth, paddleHeight, 0xFFFF);
  display.tft.fillRect(paddle2X, paddle2Y, paddleWidth, paddleHeight, 0xFFFF);
  display.tft.fillCircle(ballX, ballY, 3, 0xFFFF);

  display.tft.setTextColor(0xFFFF, 0x0000);
  display.tft.setTextSize(2);
  display.tft.setCursor(100, 10);
  display.tft.printf("%d", score1);
  display.tft.setCursor(200, 10);
  display.tft.printf("%d", score2);
}

void AudioModeControllerPong::updateGame()
{
  // Move ball
  ballX += ballXSpeed;
  ballY += ballYSpeed;

  // Read potentiometer values for paddles and invert controls
  paddle1Y = map(i2c.getIOState().volume, 0, 255, 240 - paddleHeight - paddleBottomOffset, paddleTopOffset);
  paddle2Y = map(i2c.getIOState().tone, 0, 255, 240 - paddleHeight - paddleBottomOffset, paddleTopOffset);

  // Keep paddle in bound (using top and bottom offset)
  paddle1Y = std::max(paddleTopOffset, std::min(paddle1Y, 240 - paddleHeight - paddleBottomOffset));
  paddle2Y = std::max(paddleTopOffset, std::min(paddle2Y, 240 - paddleHeight - paddleBottomOffset));

  handleCollisions();
}

void AudioModeControllerPong::resetBall()
{
  ballX = 160;
  ballY = 120;
  ballXSpeed = initialBallXSpeed;
  ballYSpeed = (random(0, 2) == 0) ? -initialBallYSpeed : initialBallYSpeed;
}

void AudioModeControllerPong::handleCollisions()
{
  // Bounce off top/bottom walls (using top and bottom offsets)
  if (ballY <= paddleTopOffset || ballY >= 240 - paddleBottomOffset)
  {
    ballYSpeed = -ballYSpeed;
    audio.getMemoryPlayer()->play(beep);
  }

  int paddle1X = paddleSideOffset;
  int paddle2X = 320 - paddleWidth - paddleSideOffset;

  // Check collision with left paddle
  if (ballX - 3 <= paddle1X + paddleWidth && ballX >= paddle1X && ballY >= paddle1Y && ballY <= paddle1Y + paddleHeight)
  {
    ballXSpeed = -ballXSpeed;
    increaseBallSpeed();
    audio.getMemoryPlayer()->play(beep);
  }

  // Check collision with right paddle
  if (ballX + 3 >= paddle2X && ballX <= paddle2X + paddleWidth && ballY >= paddle2Y && ballY <= paddle2Y + paddleHeight)
  {
    ballXSpeed = -ballXSpeed;
    increaseBallSpeed();
    audio.getMemoryPlayer()->play(beep);
  }

  // Check if ball passed a paddle (scoring)
  if (ballX < paddleSideOffset)
  { // check for ball going past left bound (paddle offset)
    score2++;
    resetBall();
    audio.getMemoryPlayer()->play(boop);
  }
  else if (ballX > 320 - paddleSideOffset)
  { // check for ball going past right bound (paddle offset)
    score1++;
    resetBall();
    audio.getMemoryPlayer()->play(boop);
  }
}

void AudioModeControllerPong::increaseBallSpeed()
{
  // Increase speed by a percentage and round to nearest integer
  ballXSpeed = round(ballXSpeed * ballSpeedIncreaseFactor);
  ballYSpeed = round(ballYSpeed * ballSpeedIncreaseFactor);

  // Limit the maximum speed to prevent infinite increase
  ballXSpeed = constrain(ballXSpeed, -maxBallXSpeed, maxBallXSpeed);
  ballYSpeed = constrain(ballYSpeed, -maxBallYSpeed, maxBallYSpeed);
}
