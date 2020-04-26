#include <algorithm>

#include "VoxelInstance.h"

#include "components/debug/Debug.h"

void VoxelInstance::DoorState::init(double speed, double percentOpen, StateType stateType)
{
	if (stateType == StateType::Closed)
	{
		DebugAssert(percentOpen == 0.0);
	}
	else if (stateType == StateType::Open)
	{
		DebugAssert(percentOpen == 1.0);
	}

	this->speed = speed;
	this->percentOpen = percentOpen;
	this->stateType = stateType;
}

double VoxelInstance::DoorState::getSpeed() const
{
	return this->speed;
}

double VoxelInstance::DoorState::getPercentOpen() const
{
	return this->percentOpen;
}

VoxelInstance::DoorState::StateType VoxelInstance::DoorState::getStateType() const
{
	return this->stateType;
}

void VoxelInstance::DoorState::setStateType(StateType stateType)
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

void VoxelInstance::DoorState::update(double dt)
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

void VoxelInstance::FadeState::init(double speed, double percentFaded)
{
	this->speed = speed;
	this->percentFaded = percentFaded;
}

double VoxelInstance::FadeState::getSpeed() const
{
	return this->speed;
}

double VoxelInstance::FadeState::getPercentFaded() const
{
	return this->percentFaded;
}

bool VoxelInstance::FadeState::isDoneFading() const
{
	return this->percentFaded == 1.0;
}

void VoxelInstance::FadeState::update(double dt)
{
	if (!this->isDoneFading())
	{
		const double delta = this->speed * dt;
		this->percentFaded = std::clamp(this->percentFaded + delta, this->percentFaded, 1.0);
	}
}

void VoxelInstance::init(WEInt x, int y, SNInt z, Type type)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
}

VoxelInstance VoxelInstance::makeDoor(WEInt x, int y, SNInt z, double speed, double percentOpen,
	DoorState::StateType stateType)
{
	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::OpenDoor);
	voxelInst.door.init(speed, percentOpen, stateType);
	return voxelInst;
}

VoxelInstance VoxelInstance::makeDoor(WEInt x, int y, SNInt z, double speed)
{
	constexpr double percentOpen = 0.0;
	constexpr DoorState::StateType stateType = DoorState::StateType::Opening;
	return VoxelInstance::makeDoor(x, y, z, speed, percentOpen, stateType);
}

VoxelInstance VoxelInstance::makeFading(WEInt x, int y, SNInt z, double speed, double percentFaded)
{
	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::Fading);
	voxelInst.fade.init(speed, percentFaded);
	return voxelInst;
}

VoxelInstance VoxelInstance::makeFading(WEInt x, int y, SNInt z, double speed)
{
	constexpr double percentFaded = 0.0;
	return VoxelInstance::makeFading(x, y, z, speed, percentFaded);
}

WEInt VoxelInstance::getX() const
{
	return this->x;
}

int VoxelInstance::getY() const
{
	return this->y;
}

SNInt VoxelInstance::getZ() const
{
	return this->z;
}

VoxelInstance::Type VoxelInstance::getType() const
{
	return this->type;
}

VoxelInstance::DoorState &VoxelInstance::getDoorState()
{
	DebugAssert(this->type == Type::OpenDoor);
	return this->door;
}

const VoxelInstance::DoorState &VoxelInstance::getDoorState() const
{
	DebugAssert(this->type == Type::OpenDoor);
	return this->door;
}

VoxelInstance::FadeState &VoxelInstance::getFadeState()
{
	DebugAssert(this->type == Type::Fading);
	return this->fade;
}

const VoxelInstance::FadeState &VoxelInstance::getFadeState() const
{
	DebugAssert(this->type == Type::Fading);
	return this->fade;
}

void VoxelInstance::update(double dt)
{
	if (this->type == Type::OpenDoor)
	{
		this->door.update(dt);
	}
	else if (this->type == Type::Fading)
	{
		this->fade.update(dt);
	}
}
