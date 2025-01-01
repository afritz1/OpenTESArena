#include "VoxelDoorDefinition.h"

void VoxelDoorOpenSoundDefinition::init(const std::string &soundFilename)
{
	this->soundFilename = soundFilename;
}

void VoxelDoorCloseSoundDefinition::init(VoxelDoorCloseType closeType, const std::string &soundFilename)
{
	this->closeType = closeType;
	this->soundFilename = soundFilename;
}

VoxelDoorDefinition::VoxelDoorDefinition()
{
	this->type = static_cast<ArenaTypes::DoorType>(-1);
}

void VoxelDoorDefinition::init(ArenaTypes::DoorType type, const std::string &openSoundFilename, VoxelDoorCloseType closeType, const std::string &closeSoundFilename)
{
	this->type = type;
	this->openSoundDef.init(openSoundFilename);
	this->closeSoundDef.init(closeType, closeSoundFilename);
}
