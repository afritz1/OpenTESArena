#include "DoorDefinition.h"

void DoorDefinition::OpenSoundDef::init(std::string &&soundFilename)
{
	this->soundFilename = std::move(soundFilename);
}

void DoorDefinition::CloseSoundDef::init(DoorDefinition::CloseType closeType, std::string &&soundFilename)
{
	this->closeType = closeType;
	this->soundFilename = std::move(soundFilename);
}

DoorDefinition::DoorDefinition()
{
	this->type = static_cast<ArenaTypes::DoorType>(-1);
}

void DoorDefinition::init(ArenaTypes::DoorType type, std::string &&openSoundFilename, CloseType closeType,
	std::string &&closeSoundFilename)
{
	this->type = type;
	this->openSoundDef.init(std::move(openSoundFilename));
	this->closeSoundDef.init(closeType, std::move(closeSoundFilename));
}

ArenaTypes::DoorType DoorDefinition::getType() const
{
	return this->type;
}

const DoorDefinition::OpenSoundDef &DoorDefinition::getOpenSound() const
{
	return this->openSoundDef;
}

const DoorDefinition::CloseSoundDef &DoorDefinition::getCloseSound() const
{
	return this->closeSoundDef;
}
