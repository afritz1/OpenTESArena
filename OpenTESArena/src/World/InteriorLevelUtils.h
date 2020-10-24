#ifndef INTERIOR_LEVEL_UTILS_H
#define INTERIOR_LEVEL_UTILS_H

#include "VoxelUtils.h"

namespace InteriorLevelUtils
{
	// Number of voxels high all interiors are.
	constexpr int GRID_HEIGHT = 3;

	// Width/depth of dungeon chunks in voxels.
	constexpr int DUNGEON_CHUNK_DIM = 32;

	// Packs a *LEVELUP or *LEVELDOWN voxel into a transition ID.
	int packLevelChangeVoxel(WEInt x, SNInt y);

	// Unpacks a transition ID into X and Y voxel offsets.
	void unpackLevelChangeVoxel(int voxel, WEInt *outX, SNInt *outY);

	// Moves a level change voxel (in a dungeon) by a fixed amount. The given coordinate can be
	// either an X or Z value and should be unpacked.
	int offsetLevelChangeVoxel(int coord);

	// Converts a level change voxel to an actual level voxel.
	uint16_t convertLevelChangeVoxel(uint8_t voxel);

	// Converts an Arena ceiling height from "centimeters" to modern coordinates (1.0 per voxel).
	double convertArenaCeilingHeight(int ceilingHeight);
}

#endif
