#include "VoxelDoorDefinition.h"

void VoxelDoorDefinition::OpenSoundDef::init(std::string &&soundFilename)
{
	this->soundFilename = std::move(soundFilename);
}

void VoxelDoorDefinition::CloseSoundDef::init(VoxelDoorDefinition::CloseType closeType, std::string &&soundFilename)
{
	this->closeType = closeType;
	this->soundFilename = std::move(soundFilename);
}

VoxelDoorDefinition::VoxelDoorDefinition()
{
	this->type = static_cast<ArenaTypes::DoorType>(-1);
}

void VoxelDoorDefinition::init(ArenaTypes::DoorType type, std::string &&openSoundFilename, CloseType closeType,
	std::string &&closeSoundFilename)
{
	this->type = type;
	this->openSoundDef.init(std::move(openSoundFilename));
	this->closeSoundDef.init(closeType, std::move(closeSoundFilename));
}

ArenaTypes::DoorType VoxelDoorDefinition::getType() const
{
	return this->type;
}

const VoxelDoorDefinition::OpenSoundDef &VoxelDoorDefinition::getOpenSound() const
{
	return this->openSoundDef;
}

const VoxelDoorDefinition::CloseSoundDef &VoxelDoorDefinition::getCloseSound() const
{
	return this->closeSoundDef;
}
