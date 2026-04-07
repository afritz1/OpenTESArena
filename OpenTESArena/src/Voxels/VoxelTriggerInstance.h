#pragma once

#include "../World/Coord.h"

// Simply a record that a trigger occurred at a voxel coordinate. Intended for one-shot *TEXT triggers.
struct VoxelTriggerInstance
{
	SNInt x;
	int y;
	WEInt z;

	VoxelTriggerInstance();

	void init(SNInt x, int y, WEInt z);
};
