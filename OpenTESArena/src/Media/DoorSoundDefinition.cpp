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

void DoorSoundDefinition::init(std::string &&openSoundFilename, CloseType closeType, std::string &&closeSoundFilename)
{
	this->open.init(std::move(openSoundFilename));
	this->close.init(closeType, std::move(closeSoundFilename));
}

const DoorSoundDefinition::OpenDef &DoorSoundDefinition::getOpen() const
{
	return this->open;
}

const DoorSoundDefinition::CloseDef &DoorSoundDefinition::getClose() const
{
	return this->close;
}
