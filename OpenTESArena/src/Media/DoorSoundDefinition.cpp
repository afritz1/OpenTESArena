#include "DoorSoundDefinition.h"

#include "components/debug/Debug.h"

void DoorSoundDefinition::OpenDef::init(std::string &&soundFilename)
{
	this->soundFilename = std::move(soundFilename);
}

void DoorSoundDefinition::CloseDef::init(CloseType closeType, std::string &&soundFilename)
{
	this->closeType = closeType;
	this->soundFilename = std::move(soundFilename);
}

DoorSoundDefinition::DoorSoundDefinition()
{
	this->doorType = static_cast<ArenaTypes::DoorType>(-1);
}

void DoorSoundDefinition::init(ArenaTypes::DoorType doorType, std::string &&openSoundFilename,
	CloseType closeType, std::string &&closeSoundFilename)
{
	this->doorType = doorType;
	this->open.init(std::move(openSoundFilename));
	this->close.init(closeType, std::move(closeSoundFilename));
}

ArenaTypes::DoorType DoorSoundDefinition::getDoorType() const
{
	return this->doorType;
}

const DoorSoundDefinition::OpenDef &DoorSoundDefinition::getOpen() const
{
	return this->open;
}

const DoorSoundDefinition::CloseDef &DoorSoundDefinition::getClose() const
{
	return this->close;
}
