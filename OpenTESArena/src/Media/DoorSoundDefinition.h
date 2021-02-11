#ifndef DOOR_SOUND_DEFINITION_H
#define DOOR_SOUND_DEFINITION_H

#include "../Assets/ArenaTypes.h"
#include "../World/VoxelDefinition.h"

class DoorSoundDefinition
{
public:
	enum class Type
	{
		Open,
		Close
	};

	// Each door has a certain behavior for playing sounds when closing.
	enum class CloseType
	{
		OnClosed,
		OnClosing
	};

	struct OpenDef
	{
		int soundIndex;

		void init(int soundIndex);
	};

	struct CloseDef
	{
		CloseType closeType;
		int soundIndex;

		void init(CloseType closeType, int soundIndex);
	};
private:
	ArenaTypes::DoorType doorType;
	Type type;
	OpenDef open;
	CloseDef close;

	void init(ArenaTypes::DoorType doorType, Type type);
public:
	DoorSoundDefinition();

	void initOpen(ArenaTypes::DoorType doorType, int soundIndex);
	void initClose(ArenaTypes::DoorType doorType, CloseType closeType, int soundIndex);

	ArenaTypes::DoorType getDoorType() const;
	Type getType() const;
	const OpenDef &getOpen() const;
	const CloseDef &getClose() const;
};

#endif
