#ifndef TIMER_H
#define TIMER_H

class Timer
{
private:
	double currentSeconds, targetSeconds;
public:
	Timer(double targetSeconds);

	double getCurrentSeconds() const;
	double getTargetSeconds() const;

	// Gets the timer progress as a 0->1 percent.
	double getPercent() const;

	// Returns whether the elapsed time has matched or passed the target seconds.
	bool isDone() const;

	// Subtracts the target seconds from the current seconds (useful for preserving 
	// total time between partial-resets).
	void subtractTarget();

	// Resets the current seconds to zero.
	void reset();

	// Ticks the timer by delta time.
	void tick(double dt);
};

#endif
