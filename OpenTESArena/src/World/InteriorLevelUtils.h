#ifndef INTERIOR_LEVEL_UTILS_H
#define INTERIOR_LEVEL_UTILS_H

#include "VoxelUtils.h"

namespace InteriorLevelUtils
{
	// Number of voxels high all interiors are.
	constexpr int GRID_HEIGHT = 3;

	// Width and depth of dungeon chunks in voxels.
	constexpr WEInt DUNGEON_CHUNK_WIDTH = 32;
	constexpr SNInt DUNGEON_CHUNK_DEPTH = DUNGEON_CHUNK_WIDTH;
}

#endif
