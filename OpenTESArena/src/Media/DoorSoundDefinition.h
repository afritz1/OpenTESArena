#ifndef DOOR_SOUND_DEFINITION_H
#define DOOR_SOUND_DEFINITION_H

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
	VoxelDefinition::DoorData::Type doorType;
	Type type;
	OpenDef open;
	CloseDef close;

	void init(VoxelDefinition::DoorData::Type doorType, Type type);
public:
	DoorSoundDefinition();

	void initOpen(VoxelDefinition::DoorData::Type doorType, int soundIndex);
	void initClose(VoxelDefinition::DoorData::Type doorType, CloseType closeType, int soundIndex);

	VoxelDefinition::DoorData::Type getDoorType() const;
	Type getType() const;
	const OpenDef &getOpen() const;
	const CloseDef &getClose() const;
};

#endif
