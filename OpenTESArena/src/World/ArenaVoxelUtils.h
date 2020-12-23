#ifndef ARENA_VOXEL_UTILS_H
#define ARENA_VOXEL_UTILS_H

#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFUtils.h"

enum class WorldType;

namespace ArenaVoxelUtils
{
	// Original game ID values.
	static constexpr int TOTAL_VOXEL_IDS = 64;

	// The size of wet chasms and lava chasms, unaffected by ceiling height.
	constexpr double WET_CHASM_DEPTH = static_cast<double>(
		INFFile::CeilingData::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

	// Seconds per chasm animation loop.
	constexpr double CHASM_ANIM_SECONDS = 1.0 / 2.0; // @todo: arbitrary, get original game value.

	// Gets exterior menu type from *MENU ID and world type, or "none" if no mapping exists.
	ArenaTypes::MenuType getMenuType(int menuID, WorldType worldType);

	// Returns whether the menu type is for an interior (equipment, tavern, etc.) or something
	// else (like city gates).
	bool menuLeadsToInterior(ArenaTypes::MenuType menuType);

	// Returns whether the menu type displays text on-screen when the player right clicks it.
	bool menuHasDisplayName(ArenaTypes::MenuType menuType);
}

#endif
