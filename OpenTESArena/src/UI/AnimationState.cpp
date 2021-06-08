#include <algorithm>
#include <cmath>

#include "AnimationState.h"

AnimationState::AnimationState()
{
	this->targetSeconds = 0.0;
	this->currentSeconds = 0.0;
	this->looping = false;
}

void AnimationState::init(double targetSeconds, bool looping, std::function<void()> &&onFinished)
{
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
	this->looping = looping;
	this->onFinished = std::move(onFinished);
}

void AnimationState::init(double targetSeconds, bool looping)
{
	this->init(targetSeconds, looping, []() { });
}

double AnimationState::getPercent() const
{
	return std::clamp(this->currentSeconds / this->targetSeconds, 0.0, 1.0);
}

void AnimationState::reset()
{
	this->currentSeconds = 0.0;
}

void AnimationState::update(double dt)
{
	if (this->looping)
	{
		this->currentSeconds = std::fmod(this->currentSeconds + dt, this->targetSeconds);
	}
	else
	{
		if (this->currentSeconds < this->targetSeconds)
		{
			this->currentSeconds += dt;
			if (this->currentSeconds >= this->targetSeconds)
			{
				this->onFinished();
			}
		}
	}
}
