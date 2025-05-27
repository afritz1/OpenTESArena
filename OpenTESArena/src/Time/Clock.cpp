#include "Clock.h"

#include "components/debug/Debug.h"

void Clock::init(int hours, int minutes, int seconds, double currentSecond)
{
	DebugAssert(hours >= 0);
	DebugAssert(hours < 24);
	DebugAssert(minutes >= 0);
	DebugAssert(minutes < 60);
	DebugAssert(seconds >= 0);
	DebugAssert(seconds < 60);
	DebugAssert(currentSecond >= 0.0);

	this->hours = hours;
	this->minutes = minutes;
	this->seconds = seconds;
	this->currentSecond = currentSecond;
}

void Clock::init(int hours, int minutes, int seconds)
{
	this->init(hours, minutes, seconds, 0.0);
}

void Clock::clear()
{
	this->init(0, 0, 0, 0.0);
}

int Clock::getHours12() const
{
	const int hoursMod = this->hours % 12;
	return (hoursMod == 0) ? 12 : hoursMod;
}

double Clock::getTotalSeconds() const
{
	const int seconds = (this->hours * 3600) + (this->minutes * 60) + this->seconds;
	return static_cast<double>(seconds) + this->currentSecond;
}

double Clock::getDayPercent() const
{
	return this->getTotalSeconds() / static_cast<double>(Clock::SECONDS_IN_A_DAY);
}

bool Clock::isAM() const
{
	return this->hours < 12;
}

void Clock::incrementHour()
{
	this->hours++;

	if (this->hours == 24)
	{
		this->hours = 0;
	}
}

void Clock::incrementMinute()
{
	this->minutes++;

	if (this->minutes == 60)
	{
		this->incrementHour();
		this->minutes = 0;
	}
}

void Clock::incrementSecond()
{
	this->seconds++;

	if (this->seconds == 60)
	{
		this->incrementMinute();
		this->seconds = 0;
	}
}

void Clock::incrementTime(double dt)
{
	this->currentSecond += dt;

	while (this->currentSecond >= 1.0)
	{
		this->incrementSecond();
		this->currentSecond -= 1.0;
	}
}
