#include <algorithm>

#include "VoxelFacing3D.h"
#include "VoxelInstance.h"

#include "components/debug/Debug.h"

void VoxelInstance::ChasmState::init(bool north, bool east, bool south, bool west)
{
	this->north = north;
	this->east = east;
	this->south = south;
	this->west = west;
}

bool VoxelInstance::ChasmState::getNorth() const
{
	return this->north;
}

bool VoxelInstance::ChasmState::getEast() const
{
	return this->east;
}

bool VoxelInstance::ChasmState::getSouth() const
{
	return this->south;
}

bool VoxelInstance::ChasmState::getWest() const
{
	return this->west;
}

bool VoxelInstance::ChasmState::faceIsVisible(VoxelFacing3D facing) const
{
	switch (facing)
	{
	case VoxelFacing3D::PositiveX:
		return this->south;
	case VoxelFacing3D::PositiveZ:
		return this->west;
	case VoxelFacing3D::NegativeX:
		return this->north;
	case VoxelFacing3D::NegativeZ:
		return this->east;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(facing)));
	}
}

bool VoxelInstance::ChasmState::faceIsVisible(VoxelFacing2D facing) const
{
	return this->faceIsVisible(VoxelUtils::convertFaceTo3D(facing));
}

int VoxelInstance::ChasmState::getFaceCount() const
{
	// Add one for floor.
	return 1 + (this->north ? 1 : 0) + (this->east ? 1 : 0) +
		(this->south ? 1 : 0) + (this->west ? 1 : 0);
}

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

VoxelInstance::VoxelInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->type = static_cast<VoxelInstance::Type>(-1);
}

void VoxelInstance::init(SNInt x, int y, WEInt z, Type type)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
}

VoxelInstance VoxelInstance::makeChasm(SNInt x, int y, WEInt z, bool north, bool east,
	bool south, bool west)
{
	// A chasm with no walls does not need a voxel instance.
	DebugAssert(north || east || south || west);

	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::Chasm);
	voxelInst.chasm.init(north, east, south, west);
	return voxelInst;
}

VoxelInstance VoxelInstance::makeDoor(SNInt x, int y, WEInt z, double speed, double percentOpen,
	DoorState::StateType stateType)
{
	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::OpenDoor);
	voxelInst.door.init(speed, percentOpen, stateType);
	return voxelInst;
}

VoxelInstance VoxelInstance::makeDoor(SNInt x, int y, WEInt z, double speed)
{
	constexpr double percentOpen = 0.0;
	constexpr DoorState::StateType stateType = DoorState::StateType::Opening;
	return VoxelInstance::makeDoor(x, y, z, speed, percentOpen, stateType);
}

VoxelInstance VoxelInstance::makeFading(SNInt x, int y, WEInt z, double speed, double percentFaded)
{
	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::Fading);
	voxelInst.fade.init(speed, percentFaded);
	return voxelInst;
}

VoxelInstance VoxelInstance::makeFading(SNInt x, int y, WEInt z, double speed)
{
	constexpr double percentFaded = 0.0;
	return VoxelInstance::makeFading(x, y, z, speed, percentFaded);
}

SNInt VoxelInstance::getX() const
{
	return this->x;
}

int VoxelInstance::getY() const
{
	return this->y;
}

WEInt VoxelInstance::getZ() const
{
	return this->z;
}

VoxelInstance::Type VoxelInstance::getType() const
{
	return this->type;
}

VoxelInstance::ChasmState &VoxelInstance::getChasmState()
{
	DebugAssert(this->type == Type::Chasm);
	return this->chasm;
}

const VoxelInstance::ChasmState &VoxelInstance::getChasmState() const
{
	DebugAssert(this->type == Type::Chasm);
	return this->chasm;
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

bool VoxelInstance::hasRelevantState() const
{
	if (this->type == Type::Chasm)
	{
		const VoxelInstance::ChasmState &chasmState = this->chasm;
		return chasmState.getNorth() || chasmState.getSouth() || chasmState.getEast() || chasmState.getWest();
	}
	else if (this->type == Type::OpenDoor)
	{
		const VoxelInstance::DoorState &doorState = this->door;
		return doorState.getStateType() != VoxelInstance::DoorState::StateType::Closed;
	}
	else if (this->type == Type::Fading)
	{
		const VoxelInstance::FadeState &fadeState = this->fade;
		return !fadeState.isDoneFading();
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
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
