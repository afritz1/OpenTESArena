#include <algorithm>

#include "VoxelDoorAnimationInstance.h"

#include "components/debug/Debug.h"

VoxelDoorAnimationInstance::VoxelDoorAnimationInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->speed = 0.0;
	this->percentOpen = 0.0;
	this->stateType = VoxelDoorAnimationStateType::Closed;
}

void VoxelDoorAnimationInstance::init(SNInt x, int y, WEInt z, double speed, double percentOpen, VoxelDoorAnimationStateType stateType)
{
	if (stateType == VoxelDoorAnimationStateType::Closed)
	{
		DebugAssert(percentOpen == 0.0);
	}
	else if (stateType == VoxelDoorAnimationStateType::Open)
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

void VoxelDoorAnimationInstance::initOpening(SNInt x, int y, WEInt z, double speed)
{
	this->init(x, y, z, speed, 0.0, VoxelDoorAnimationStateType::Opening);
}

void VoxelDoorAnimationInstance::setStateType(VoxelDoorAnimationStateType stateType)
{
	this->stateType = stateType;

	if (stateType == VoxelDoorAnimationStateType::Closed)
	{
		this->percentOpen = 0.0;
	}
	else if (stateType == VoxelDoorAnimationStateType::Open)
	{
		this->percentOpen = 1.0;
	}
}

void VoxelDoorAnimationInstance::update(double dt)
{
	const double delta = this->speed * dt;

	if (this->stateType == VoxelDoorAnimationStateType::Opening)
	{
		this->percentOpen = std::clamp(this->percentOpen + delta, this->percentOpen, 1.0);

		if (this->percentOpen == 1.0)
		{
			this->stateType = VoxelDoorAnimationStateType::Open;
		}
	}
	else if (this->stateType == VoxelDoorAnimationStateType::Closing)
	{
		this->percentOpen = std::clamp(this->percentOpen - delta, 0.0, this->percentOpen);

		if (this->percentOpen == 0.0)
		{
			this->stateType = VoxelDoorAnimationStateType::Closed;
		}
	}
}
