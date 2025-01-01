#ifndef DOOR_DEFINITION_H
#define DOOR_DEFINITION_H

#include <string>

#include "../Assets/ArenaTypes.h"

class VoxelDoorDefinition
{
public:
	// Each door has a certain behavior for playing sounds when closing.
	enum class CloseType
	{
		OnClosed,
		OnClosing
	};

	struct OpenSoundDef
	{
		std::string soundFilename;

		void init(std::string &&soundFilename);
	};

	struct CloseSoundDef
	{
		CloseType closeType;
		std::string soundFilename;

		void init(CloseType closeType, std::string &&soundFilename);
	};
private:
	ArenaTypes::DoorType type;
	OpenSoundDef openSoundDef;
	CloseSoundDef closeSoundDef;
public:
	VoxelDoorDefinition();

	void init(ArenaTypes::DoorType type, std::string &&openSoundFilename, CloseType closeType,
		std::string &&closeSoundFilename);

	ArenaTypes::DoorType getType() const;
	const OpenSoundDef &getOpenSound() const;
	const CloseSoundDef &getCloseSound() const;
};

#endif
