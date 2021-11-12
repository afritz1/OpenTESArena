#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <array>

class FPSCounter
{
private:
	std::array<double, 60> frameTimes;

	// Gets the average frame time in seconds based on recent data.
	double getAverageFrameTime() const;
public:
	FPSCounter();

	// Gets the number of frame times the counter can store.
	int getFrameCount() const;

	// Gets the time in seconds of a particular frame in the counter's history.
	double getFrameTime(int index) const;

	double getAverageFPS() const;
	double getHighestFPS() const;
	double getLowestFPS() const;

	// Sets the frame time of the most recent frame. This should be called once
	// per frame.
	void updateFrameTime(double dt);
};

#endif
