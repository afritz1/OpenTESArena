#ifndef ARENA_VOXEL_UTILS_H
#define ARENA_VOXEL_UTILS_H

#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFUtils.h"

enum class MapType;

namespace ArenaVoxelUtils
{
	// Original game ID values.
	static constexpr int TOTAL_VOXEL_IDS = 64;

	// The size of wet chasms and lava chasms, unaffected by ceiling height.
	constexpr double WET_CHASM_DEPTH = static_cast<double>(
		INFFile::CeilingData::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

	// Seconds per chasm animation loop.
	constexpr double CHASM_ANIM_SECONDS = 1.0 / 2.0; // @todo: arbitrary, get original game value.

	// Seconds for a door to open.
	//constexpr double DOOR_ANIM_SECONDS = 1.0 / 1.30; // @todo: arbitrary value
	constexpr double DOOR_ANIM_SPEED = 1.30; // @todo: change to animation seconds

	// Seconds for a fading voxel to animate.
	constexpr double FADING_VOXEL_SECONDS = 1.0;

	// Gets exterior menu type from *MENU ID and world type, or "none" if no mapping exists.
	ArenaTypes::MenuType getMenuType(int menuID, MapType mapType);

	// Returns whether the menu type is for an interior (equipment, tavern, etc.) or something
	// else (like city gates).
	bool menuLeadsToInterior(ArenaTypes::MenuType menuType);

	// Returns whether the menu type displays text on-screen when the player right clicks it.
	bool menuHasDisplayName(ArenaTypes::MenuType menuType);

	// Validates a voxel texture ID to make sure it's in the proper range and clamps if necessary.
	int clampVoxelTextureID(int id);

	// Gets the texture filename for the given voxel texture ID.
	std::string getVoxelTextureFilename(int id, const INFFile &inf);

	// Gets the index into a texture set for the given voxel texture ID, if any. For example, it may
	// return 2 in a 4-image .SET file, or none if not a .SET file.
	std::optional<int> getVoxelTextureSetIndex(int id, const INFFile &inf);

	// Returns whether the floor would be colored like a wall on the wild automap, to make it easier
	// to see roads, etc..
	bool isFloorWildWallColored(int floorID, MapType mapType);
}

#endif
