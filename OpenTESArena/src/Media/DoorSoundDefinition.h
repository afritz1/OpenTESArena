#ifndef DOOR_SOUND_DEFINITION_H
#define DOOR_SOUND_DEFINITION_H

#include <string>

#include "../Assets/ArenaTypes.h"
#include "../World/VoxelDefinition.h"

class DoorSoundDefinition
{
public:
	// Each door has a certain behavior for playing sounds when closing.
	enum class CloseType
	{
		OnClosed,
		OnClosing
	};

	struct OpenDef
	{
		std::string soundFilename;

		void init(std::string &&soundFilename);
	};

	struct CloseDef
	{
		CloseType closeType;
		std::string soundFilename;

		void init(CloseType closeType, std::string &&soundFilename);
	};
private:
	ArenaTypes::DoorType doorType;
	OpenDef open;
	CloseDef close;
public:
	DoorSoundDefinition();

	void init(ArenaTypes::DoorType doorType, std::string &&openSoundFilename, CloseType closeType,
		std::string &&closeSoundFilename);

	ArenaTypes::DoorType getDoorType() const;
	const OpenDef &getOpen() const;
	const CloseDef &getClose() const;
};

#endif
