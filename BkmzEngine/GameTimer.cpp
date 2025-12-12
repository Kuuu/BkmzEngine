#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer()
	: secondsPerCount(0.0), deltaTime(-1.0), baseTime(0),
	pausedTime(0), stopTime(0), prevTime(0), currTime(0), stopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER *)&countsPerSec);
	secondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::GameTime() const
{
	return 0.0f;
}

float GameTimer::DeltaTime() const
{
	return (float)deltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
	baseTime = currTime;
	prevTime = currTime;
	stopTime = 0;
	stopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&startTime);

	// If we are resuming the timer from a stopped state...
	if (stopped)
	{
		// then accumulate the paused time.
		pausedTime += (startTime - stopTime);
		// since we are starting the timer back up, the current
		// previous time is not valid, as it occurred while paused.
		// So reset it to the current time.
		prevTime = startTime;
		// no longer stopped...
		stopTime = 0;
		stopped = false;
	}
}

void GameTimer::Stop()
{
	// If we are already stopped, then donÅft do anything.
	if (!stopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
		// Otherwise, save the time we stopped at, and set
		// the Boolean flag indicating the timer is stopped.
		stopTime = currTime;
		stopped = true;
	}
}

void GameTimer::Tick()
{
	if (stopped)
	{
		deltaTime = 0.0;
		return;
	}

	// Get the time this frame.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
	this->currTime = currTime;

	// Time difference between this frame and the previous.
	deltaTime = (currTime - prevTime) * secondsPerCount;

	// Prepare for next frame.
	prevTime = currTime;

	// Force nonnegative. The DXSDKÅfs CDXUTTimer mentions that if the
	// processor goes into a power save mode or we get shuffled to
	// another processor, mDeltaTime might be negative.
	if (deltaTime < 0.0)
	{
		deltaTime = 0.0;
	}
}

float GameTimer::TotalTime() const
{
	// If we are stopped, do not count the time that has passed
	// since we stopped. Moreover, if we previously already had
	// a pause, the distance mStopTime - mBaseTime includes paused
	// time,which we do not want to count. To correct this, we can
	// subtract the paused time from mStopTime:
	//
	// previous paused time
	// |<----------->|
	// ---*------------*-------------*-------*-----------*------> time
	// mBaseTime mStopTime mCurrTime
	if (stopped)
	{
		return (float)(((stopTime - pausedTime) -
			baseTime) * secondsPerCount);
	}
	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count. To correct this, we can subtract
	// the paused time from mCurrTime:
	//
	// (mCurrTime - mPausedTime) - mBaseTime
	//
	// |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	// mBaseTime mStopTime startTime mCurrTime
	else
	{
		return (float)(((currTime - pausedTime) -
			baseTime) * secondsPerCount);
	}
}