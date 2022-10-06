#include <algorithm>

#include "DoorAnimationInstance.h"

#include "components/debug/Debug.h"

DoorAnimationInstance::DoorAnimationInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->speed = 0.0;
	this->percentOpen = 0.0;
	this->stateType = StateType::Closed;
}

void DoorAnimationInstance::init(SNInt x, int y, WEInt z, double speed, double percentOpen, StateType stateType)
{
	if (stateType == StateType::Closed)
	{
		DebugAssert(percentOpen == 0.0);
	}
	else if (stateType == StateType::Open)
	{
		DebugAssert(percentOpen == 1.0);
	}

	this->x = x;
	this->y = y;
	this->z = z;
	this->speed = speed;
	this->percentOpen = percentOpen;
	this->stateType = stateType;
}

void DoorAnimationInstance::initOpening(SNInt x, int y, WEInt z, double speed)
{
	this->init(x, y, z, speed, 0.0, StateType::Opening);
}

void DoorAnimationInstance::setStateType(StateType stateType)
{
	this->stateType = stateType;

	if (stateType == StateType::Closed)
	{
		this->percentOpen = 0.0;
	}
	else if (stateType == StateType::Open)
	{
		this->percentOpen = 1.0;
	}
}

void DoorAnimationInstance::update(double dt)
{
	const double delta = this->speed * dt;

	if (this->stateType == StateType::Opening)
	{
		this->percentOpen = std::clamp(this->percentOpen + delta, this->percentOpen, 1.0);

		if (this->percentOpen == 1.0)
		{
			this->stateType = StateType::Open;
		}
	}
	else if (this->stateType == StateType::Closing)
	{
		this->percentOpen = std::clamp(this->percentOpen - delta, 0.0, this->percentOpen);

		if (this->percentOpen == 0.0)
		{
			this->stateType = StateType::Closed;
		}
	}
}
