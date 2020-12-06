#ifndef ARENA_VOXEL_UTILS_H
#define ARENA_VOXEL_UTILS_H

#include "../Assets/ArenaTypes.h"

enum class WorldType;

namespace ArenaVoxelUtils
{
	// Gets exterior menu type from *MENU ID and world type, or "none" if no mapping exists.
	ArenaTypes::MenuType getMenuType(int menuID, WorldType worldType);

	// Returns whether the menu type is for an interior (equipment, tavern, etc.) or something
	// else (like city gates).
	bool menuLeadsToInterior(ArenaTypes::MenuType menuType);

	// Returns whether the menu type displays text on-screen when the player right clicks it.
	bool menuHasDisplayName(ArenaTypes::MenuType menuType);
}

#endif
