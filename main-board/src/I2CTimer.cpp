#include "I2CTimer.h"
#include "Log.h"

I2CTimer::I2CTimer()
{
  firstIOPoll = 0;
  lastIOPoll = 0;
  lastBTPoll = 0;
  lastResponse = 0;
  lastRetry = 0;
  timeoutFlag = false;
  currentRetryCount = 0;
  isRetrying = false;
  fastIO = false;
}

void I2CTimer::begin()
{
  unsigned long now = millis();
  firstIOPoll = now;
  lastIOPoll = now;
  lastBTPoll = now + (BT_STATUS_INTERVAL / 3);
  lastResponse = now;
  lastRetry = 0;
  timeoutFlag = false;
  currentRetryCount = 0;
  isRetrying = false;
  lastRDSPoll = now;
  currentOperation = NONE;
  operationStartTime = 0;
}

bool I2CTimer::isWarmingUp()
{
  return millis() - firstIOPoll < WARMUP_PERIOD;
}

bool I2CTimer::shouldPollIO()
{
  if (currentOperation != NONE && currentOperation != IO_POLL)
  {
    return false;
  }
  if ((millis() - lastIOPoll) >= (fastIO ? IO_POLL_INTERVAL_FAST : IO_POLL_INTERVAL))
  {
    if (currentOperation == NONE)
    {
      currentOperation = IO_POLL;
      operationStartTime = millis();
    }
    return true;
  }
  return false;
}

bool I2CTimer::shouldPollBluetooth()
{
  if (currentOperation != NONE && currentOperation != BT_STATUS)
  {
    return false;
  }
  if ((millis() - lastBTPoll) >= BT_STATUS_INTERVAL)
  {
    if (currentOperation == NONE)
    {
      currentOperation = BT_STATUS;
      operationStartTime = millis();
    }
    return true;
  }
  return false;
}

bool I2CTimer::shouldPollRDS()
{
  if (currentOperation != NONE && currentOperation != RDS_POLL)
  {
    return false;
  }
  if ((millis() - lastRDSPoll) >= RDS_POLL_INTERVAL)
  {
    if (currentOperation == NONE)
    {
      currentOperation = RDS_POLL;
      operationStartTime = millis();
    }
    return true;
  }
  return false;
}

bool I2CTimer::hasTimeout()
{
  return timeoutFlag;
}

void I2CTimer::resetTimeout()
{
  lastResponse = millis();
  timeoutFlag = false;
}

void I2CTimer::update()
{
  unsigned long currentTime = millis();

  // Check for timeout only if we haven't had a response in TIMEOUT ms
  if (!timeoutFlag && (currentTime - lastResponse) >= TIMEOUT)
  {
    timeoutFlag = true;
  }

  // Auto-release bus if operation takes too long
  if (currentOperation != NONE)
  {
    if ((currentTime - operationStartTime) >= OPERATION_TIMEOUT)
    {
      LOG_I2C_MSG("I2C Timer: Operation timeout - releasing bus from " + String(currentOperation));
      releaseBus();
    }
  }
}

void I2CTimer::markIOPolled()
{
  lastIOPoll = millis();
}

void I2CTimer::markBTPolled()
{
  lastBTPoll = millis();
}

void I2CTimer::markRDSPolled()
{
  lastRDSPoll = millis();
}

bool I2CTimer::shouldRetry()
{
  if (!isRetrying || currentRetryCount >= MAX_RETRIES)
    return false;
  return (millis() - lastRetry) >= RETRY_INTERVAL;
}

void I2CTimer::startRetrySequence()
{
  isRetrying = true;
  currentRetryCount = 0;
  lastRetry = millis();
}

void I2CTimer::markRetryComplete()
{
  isRetrying = false;
  currentRetryCount = 0;
}
