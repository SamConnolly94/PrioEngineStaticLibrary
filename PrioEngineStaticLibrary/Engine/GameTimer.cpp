#include "GameTimer.h"

/* Constructor. */
CGameTimer::CGameTimer()
{
	// Initialise member values.
	mSecondsPerCount = 0.0;
	mDeltaTime = -1.0;
	mBaseTime = 0;
	mPausedTime = 0;
	mPrevTime = 0;
	mCurrTime = 0;
	mStopped = false;
	
	// Calculate counts per second.
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

/*
Returns the total time elapsed since Reset() was called.
Does not count any time where the clock was stopped.
*/
float CGameTimer::TotalTime() const
{
	if ( mStopped )
	{
		// Subtract any time the clock was stopped from the time elapsed to get a true reading on the time the clock has ran for.
		return static_cast<float>((( mStopTime - mPausedTime ) - mBaseTime ) * mSecondsPerCount );
	}
	else
	{
		// Subtract any pause time from full time, no need to subtract stop time as the clock isn't stopped.
		return static_cast<float>((( mCurrTime - mPausedTime ) - mBaseTime ) * mSecondsPerCount );
	}
}

/* Returns the difference in time since the timer was last called. */
float CGameTimer::DeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

/* Starts the timer again as if we were starting timing for the first time. */
void CGameTimer::Reset()
{
	// Retrieve current performance counter which can be used for time interval measurements.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	// Reset the member variables as though we were starting the clock for the first time.
	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

/* Begins timing, note only needs to be called after stopping the timer. */
void CGameTimer::Start()
{
	// Retrieve the performance counter used for time interval measurements.
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// If stopped when the start function is called.
	if ( mStopped )
	{
		// Increase the paused time to the current value.
		mPausedTime += (startTime - mStopTime);

		// Start the timer again.
		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

/* Stops the timer from running. */
void CGameTimer::Stop()
{
	// If we haven't already stopped.
	if ( !mStopped )
	{
		// Retrieve the performance counter used for time interval measurements.
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;
		mStopped = true;
	}
}

/*  Needs to be called every frame. */
void CGameTimer::Tick()
{
	// If the timer has already been stopped.
	if ( mStopped )
	{
		// Set the difference in time since last call to be none.
		mDeltaTime = 0.0;

		// Exit the function.
		return;
	}

	// Retrieve the current time from the performance counter.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// Set the current time data member of this class to the value which we just retrieved.
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrTime;

	// It is important that this time is forced to be positive, as the Direct X SDK notes
	// if a processor goes into power save mode, then mDeltaTime can be negative.
	if ( mDeltaTime < 0.0 )
	{
		mDeltaTime = 0.0;
	}
}

/* Destructor. */
CGameTimer::~CGameTimer()
{
}
