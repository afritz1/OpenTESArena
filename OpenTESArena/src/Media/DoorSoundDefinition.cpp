#include "DoorSoundDefinition.h"

#include "components/debug/Debug.h"

void DoorSoundDefinition::OpenDef::init(int soundIndex)
{
	this->soundIndex = soundIndex;
}

void DoorSoundDefinition::CloseDef::init(CloseType closeType, int soundIndex)
{
	this->closeType = closeType;
	this->soundIndex = soundIndex;
}

DoorSoundDefinition::DoorSoundDefinition()
{
	this->type = static_cast<DoorSoundDefinition::Type>(-1);
}

void DoorSoundDefinition::init(ArenaTypes::DoorType doorType, Type type)
{
	this->doorType = doorType;
	this->type = type;
}

void DoorSoundDefinition::initOpen(ArenaTypes::DoorType doorType, int soundIndex)
{
	this->init(doorType, DoorSoundDefinition::Type::Open);
	this->open.init(soundIndex);
}

void DoorSoundDefinition::initClose(ArenaTypes::DoorType doorType, CloseType closeType, int soundIndex)
{
	this->init(doorType, DoorSoundDefinition::Type::Close);
	this->close.init(closeType, soundIndex);
}

ArenaTypes::DoorType DoorSoundDefinition::getDoorType() const
{
	return this->doorType;
}

DoorSoundDefinition::Type DoorSoundDefinition::getType() const
{
	return this->type;
}

const DoorSoundDefinition::OpenDef &DoorSoundDefinition::getOpen() const
{
	DebugAssert(this->type == DoorSoundDefinition::Type::Open);
	return this->open;
}

const DoorSoundDefinition::CloseDef &DoorSoundDefinition::getClose() const
{
	DebugAssert(this->type == DoorSoundDefinition::Type::Close);
	return this->close;
}
