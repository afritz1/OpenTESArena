#include <cassert>

#include "Clock.h"

const int Clock::SECONDS_IN_A_DAY = 86400;

Clock::Clock(int hours, int minutes, int seconds, double currentSecond)
{
	// Make sure each value is in a valid range.
	assert(hours >= 0);
	assert(hours < 24);
	assert(minutes >= 0);
	assert(minutes < 60);
	assert(seconds >= 0);
	assert(seconds < 60);

	this->hours = hours;
	this->minutes = minutes;
	this->seconds = seconds;
	this->currentSecond = currentSecond;
}

Clock::Clock(int hours, int minutes, int seconds)
	: Clock(hours, minutes, seconds, 0.0) { }

Clock::Clock()
	: Clock(0, 0, 0, 0.0) { }

Clock::~Clock()
{

}

int Clock::getHours24() const
{
	return this->hours;
}

int Clock::getHours12() const
{
	const int hoursMod = this->hours % 12;
	return (hoursMod == 0) ? 12 : hoursMod;
}

int Clock::getMinutes() const
{
	return this->minutes;
}

int Clock::getSeconds() const
{
	return this->seconds;
}

double Clock::getFractionOfSecond() const
{
	return this->currentSecond;
}

int Clock::getTotalSeconds() const
{
	return (this->hours * 3600) + (this->minutes * 60) + this->seconds;
}

double Clock::getPreciseTotalSeconds() const
{
	return static_cast<double>(this->getTotalSeconds()) + this->currentSecond;
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

void Clock::tick(double dt)
{
	this->currentSecond += dt;

	while (this->currentSecond >= 1.0)
	{
		this->incrementSecond();
		this->currentSecond -= 1.0;
	}
}
