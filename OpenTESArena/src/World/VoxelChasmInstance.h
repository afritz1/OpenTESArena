#ifndef VOXEL_CHASM_INSTANCE_H
#define VOXEL_CHASM_INSTANCE_H

#include "Coord.h"

struct VoxelChasmInstance
{
	SNInt x;
	int y;
	WEInt z;
	bool north, east, south, west; // Visible faces.

	VoxelChasmInstance();

	void init(SNInt x, int y, WEInt z, bool north, bool east, bool south, bool west);

	int getFaceCount() const;
};

#endif
