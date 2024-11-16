#ifndef CLOCK_H
#define CLOCK_H

// General-purpose 24-hour clock.
struct Clock
{
	int hours; // 0 to 23
	int minutes; // 0 to 59
	int seconds; // 0 to 59
	double currentSecond; // 0 to 1

	static constexpr int SECONDS_IN_A_DAY = 86400;

	constexpr Clock(int hours, int minutes, int seconds)
	{
		this->hours = hours;
		this->minutes = minutes;
		this->seconds = seconds;
		this->currentSecond = 0.0;
	}

	constexpr Clock() : Clock(0, 0, 0) { }

	void init(int hours, int minutes, int seconds, double currentSecond);
	void init(int hours, int minutes, int seconds);
	void clear();

	// Gets hours in 12-hour format for AM/PM time.
	int getHours12() const;

	// Gets exact instant in time as seconds.
	double getTotalSeconds() const;

	// Gets how far through a day it is (0.0 = 12am, 0.50 = 12pm).
	double getDayPercent() const;

	bool isAM() const;

	void incrementHour();
	void incrementMinute();
	void incrementSecond();
	void incrementTime(double dt);
};

#endif
