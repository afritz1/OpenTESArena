#include "DoorDefinition.h"

DoorDefinition::DoorDefinition()
{
	this->type = static_cast<ArenaTypes::DoorType>(-1);
}

void DoorDefinition::init(ArenaTypes::DoorType type, DoorSoundDefinition &&doorSoundDef)
{
	this->type = type;
	this->doorSoundDef = std::move(doorSoundDef);
}

ArenaTypes::DoorType DoorDefinition::getType() const
{
	return this->type;
}

const DoorSoundDefinition &DoorDefinition::getDoorSoundDef() const
{
	return this->doorSoundDef;
}
