#ifndef DOOR_DEFINITION_H
#define DOOR_DEFINITION_H

#include <string>

#include "../Assets/ArenaTypes.h"

// Each door has a certain behavior for playing sounds when closing.
enum class VoxelDoorCloseType
{
	OnClosed,
	OnClosing
};

struct VoxelDoorOpenSoundDefinition
{
	std::string soundFilename;

	void init(const std::string &soundFilename);
};

struct VoxelDoorCloseSoundDefinition
{
	VoxelDoorCloseType closeType;
	std::string soundFilename;

	void init(VoxelDoorCloseType closeType, const std::string &soundFilename);
};

struct VoxelDoorDefinition
{
	ArenaTypes::DoorType type;
	VoxelDoorOpenSoundDefinition openSoundDef;
	VoxelDoorCloseSoundDefinition closeSoundDef;

	VoxelDoorDefinition();

	void init(ArenaTypes::DoorType type, const std::string &openSoundFilename, VoxelDoorCloseType closeType, const std::string &closeSoundFilename);
};

#endif
