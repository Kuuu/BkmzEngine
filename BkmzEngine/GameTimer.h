#pragma once

class GameTimer
{
public:
	GameTimer();
	float GameTime() const; // in seconds
	float DeltaTime() const; // in seconds
	void Reset(); // Call before message loop.
	void Start(); // Call when unpaused.
	void Stop(); // Call when paused.
	void Tick(); // Call every frame.
	float TotalTime() const;
private:
	double secondsPerCount;
	double deltaTime;
	__int64 baseTime;
	__int64 pausedTime;
	__int64 stopTime;
	__int64 prevTime;
	__int64 currTime;
	bool stopped;
};