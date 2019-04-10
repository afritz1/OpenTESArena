#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <array>

class FPSCounter
{
private:
	std::array<double, 60> frameTimes;

	// Calculates average frame time based on previous frames.
	double getAverageFrameTime() const;
public:
	FPSCounter();

	// Gets the number of frame times the counter can store.
	int getFrameCount() const;

	// Gets the time in seconds of a particular frame in the counter's history.
	double getFrameTime(int index) const;

	// Gets the average frames per second based on recent data.
	double getFPS() const;

	// Sets the frame time of the most recent frame. This should be called once
	// per frame.
	void updateFrameTime(double dt);
};

#endif
