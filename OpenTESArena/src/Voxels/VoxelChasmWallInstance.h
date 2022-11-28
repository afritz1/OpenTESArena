#ifndef VOXEL_CHASM_WALL_INSTANCE_H
#define VOXEL_CHASM_WALL_INSTANCE_H

#include "../World/Coord.h"

struct VoxelChasmWallInstance
{
	SNInt x;
	int y;
	WEInt z;
	bool north, east, south, west; // Visible faces.

	VoxelChasmWallInstance();

	void init(SNInt x, int y, WEInt z, bool north, bool east, bool south, bool west);

	int getFaceCount() const;
};

#endif
