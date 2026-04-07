#pragma once

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
