#ifndef DOOR_SOUND_LIBRARY_H
#define DOOR_SOUND_LIBRARY_H

#include <optional>
#include <vector>

#include "DoorSoundDefinition.h"
#include "../Assets/ArenaTypes.h"
#include "../World/VoxelDefinition.h"

class DoorSoundLibrary
{
private:
	std::vector<DoorSoundDefinition> defs;
public:
	void init();

	int getDefCount() const;
	const DoorSoundDefinition &getDef(int index) const;
	std::optional<int> tryGetDefIndex(ArenaTypes::DoorType doorType, DoorSoundDefinition::Type type) const;
};

#endif
