#pragma once
#include <chrono>

class FrameTimer
{
public:
	FrameTimer()
	{
		last = std::chrono::steady_clock::now();
	}


	float Mark()
	{
		const auto old = last;
		last = std::chrono::steady_clock::now();
		const std::chrono::duration<float> frameTime = last - old;
		return frameTime.count();
	}

	float Peek() const
	{
		const auto now = std::chrono::steady_clock::now();
		const std::chrono::duration<float> frameTime = now - last;
		return frameTime.count();
	}

private:
	std::chrono::steady_clock::time_point last;
};