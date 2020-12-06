#ifndef CLOCK_H
#define CLOCK_H

// General-purpose 24-hour clock.
class Clock
{
private:
	// Current hours (0->23).
	int hours;

	// Current minutes (0->59).
	int minutes;

	// Current seconds (0->59).
	int seconds;

	// Current fraction of a second (0->1).
	double currentSecond;
public:
	static constexpr int SECONDS_IN_A_DAY = 86400;

	// Starts at some time of day with the current fraction of a second for precise 
	// time definition.
	Clock(int hours, int minutes, int seconds, double currentSecond);

	// Starts at some time of day.
	Clock(int hours, int minutes, int seconds);

	// Starts at midnight.
	Clock();

	// Gets the current hours in 24-hour format.
	int getHours24() const;

	// Gets the current hours in 12-hour format (for AM/PM time).
	int getHours12() const;

	// Gets the current minutes.
	int getMinutes() const;

	// Gets the current seconds.
	int getSeconds() const;

	// Gets the current fraction of a second (between 0 and 1).
	double getFractionOfSecond() const;

	// Accumulates the current hours, minutes, and seconds into total seconds.
	int getTotalSeconds() const;

	// Combines the total seconds with the current fraction of a second for a slightly
	// more precise measurement of the current time in seconds.
	double getPreciseTotalSeconds() const;

	// Returns whether the current hour is before noon.
	bool isAM() const;

	// Increments the hour by 1.
	void incrementHour();

	// Increments the minute by 1.
	void incrementMinute();

	// Increments the second by 1.
	void incrementSecond();

	// Ticks the clock by delta time.
	void tick(double dt);
};

#endif
