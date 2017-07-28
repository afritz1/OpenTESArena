#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <array>

class FPSCounter
{
private:
	std::array<double, 20> frameTimes;

	// Calculates average frame time based on previous frames.
	double getAverageFrameTime() const;
public:
	FPSCounter();
	~FPSCounter();

	// Gets the average frames per second based on recent data.
	double getFPS() const;

	// Sets the frame time of the most recent frame. This should be called once
	// per frame.
	void updateFrameTime(double dt);
};

#endif
