#ifndef VOXEL_TRIGGER_INSTANCE_H
#define VOXEL_TRIGGER_INSTANCE_H

#include "Coord.h"

// Simply a record that a trigger occurred at a voxel coordinate. Intended for one-shot text triggers.

struct VoxelTriggerInstance
{
	SNInt x;
	int y;
	WEInt z;

	VoxelTriggerInstance();

	void init(SNInt x, int y, WEInt z);
};

#endif
