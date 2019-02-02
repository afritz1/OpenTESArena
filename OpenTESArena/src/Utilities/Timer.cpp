#include <algorithm>

#include "Timer.h"
#include "../Math/MathUtils.h"

Timer::Timer(double targetSeconds)
{
	this->currentSeconds = 0.0;
	this->targetSeconds = targetSeconds;
}

double Timer::getCurrentSeconds() const
{
	return this->currentSeconds;
}

double Timer::getTargetSeconds() const
{
	return this->targetSeconds;
}

double Timer::getPercent() const
{
	const double percent = this->currentSeconds / this->targetSeconds;
	return MathUtils::clamp(percent, 0.0, 1.0);
}

bool Timer::isDone() const
{
	return this->currentSeconds >= this->targetSeconds;
}

void Timer::subtractTarget()
{
	this->currentSeconds -= this->targetSeconds;
}

void Timer::reset()
{
	this->currentSeconds = 0.0;
}

void Timer::tick(double dt)
{
	this->currentSeconds += dt;
}
