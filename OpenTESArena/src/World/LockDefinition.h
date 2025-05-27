#ifndef LOCK_DEFINITION_H
#define LOCK_DEFINITION_H

#include "../Voxels/VoxelUtils.h"

// Supports both locks that can be picked, and locks that require a key.
struct LockDefinition
{
	SNInt x;
	int y;
	WEInt z;
	int lockLevel;
	int keyID;

	LockDefinition();

	void init(SNInt x, int y, WEInt z, int lockLevel);
};

#endif
