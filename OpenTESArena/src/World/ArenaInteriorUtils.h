#ifndef ARENA_INTERIOR_UTILS_H
#define ARENA_INTERIOR_UTILS_H

#include <cstdint>
#include <optional>
#include <string>

#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"

class ArenaRandom;

namespace ArenaInteriorUtils
{
	// Number of voxels high all interiors are.
	constexpr int GRID_HEIGHT = 3;

	// Width/depth of dungeon chunks in voxels.
	constexpr int DUNGEON_CHUNK_DIM = 32;

	// Default dungeon .MIF with chunks for random generation.
	const std::string DUNGEON_MIF_NAME = "RANDOM1.MIF";

	// Packs a *LEVELUP or *LEVELDOWN voxel into a transition ID.
	int packLevelChangeVoxel(WEInt x, SNInt y);

	// Unpacks a transition ID into X and Y voxel offsets.
	void unpackLevelChangeVoxel(int voxel, WEInt *outX, SNInt *outY);

	// Moves a level change voxel (in a dungeon) by a fixed amount. The given coordinate can be
	// either an X or Z value and should be unpacked.
	int offsetLevelChangeVoxel(int coord);

	// Converts a level change voxel to an actual level voxel.
	uint16_t convertLevelChangeVoxel(uint8_t voxel);

	// Determines how many levels a dungeon has.
	int generateDungeonLevelCount(bool isArtifactDungeon, ArenaRandom &random);

	std::optional<ArenaTypes::InteriorType> menuTypeToInteriorType(ArenaTypes::MenuType menuType);

	bool isPrefabInterior(ArenaTypes::InteriorType interiorType);
	bool isProceduralInterior(ArenaTypes::InteriorType interiorType);
}

#endif
