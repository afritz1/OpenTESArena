#include <algorithm>
#include <cmath>

#include "EntityAnimationInstance.h"

#include "components/debug/Debug.h"

EntityAnimationInstance::EntityAnimationInstance()
{
	this->clear();
}

void EntityAnimationInstance::addState(double targetSeconds, bool isLooping)
{
	if (this->stateCount >= MAX_STATES)
	{
		DebugLogError("Can't add any more states.");
		return;
	}

	this->targetSecondsList[this->stateCount] = targetSeconds;
	this->isLoopingList[this->stateCount] = isLooping;
	this->stateCount++;
}

void EntityAnimationInstance::setStateIndex(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->stateCount);
	this->currentSeconds = 0.0;
	this->targetSeconds = this->targetSecondsList[index];
	this->progressPercent = 0.0;
	this->currentStateIndex = index;
	this->isLooping = this->isLoopingList[index];
}

void EntityAnimationInstance::resetTime()
{
	this->currentSeconds = 0.0;
	this->progressPercent = 0.0;
}

void EntityAnimationInstance::clear()
{
	this->currentSeconds = 0.0;
	this->targetSeconds = 0.0;
	this->progressPercent = 0.0;
	this->currentStateIndex = -1;
	this->stateCount = 0;
	this->isLooping = false;
}

void EntityAnimationInstance::update(double dt)
{
	// @todo: maybe want to add an 'isRandom' bool to EntityAnimationDefinition::State so
	// it can more closely match citizens' animations from the original game. Either that
	// or have a separate tickRandom() method so it's more optimizable.

	if (this->isLooping)
	{
		this->currentSeconds = std::fmod(this->currentSeconds + dt, this->targetSeconds);
	}
	else
	{
		this->currentSeconds = std::min(this->currentSeconds + dt, this->targetSeconds);
	}

	this->progressPercent = std::clamp(this->currentSeconds / this->targetSeconds, 0.0, 1.0);
}
