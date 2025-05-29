#ifndef ARENA_SELECTION_UTILS_H
#define ARENA_SELECTION_UTILS_H

#include "../Assets/ArenaTypes.h"

namespace ArenaSelectionUtils
{
	// Can the given voxel type be selected with a left click?
	bool isVoxelSelectableAsPrimary(ArenaVoxelType voxelType);

	// Can the given voxel type be selected with a right click?
	bool isVoxelSelectableAsSecondary(ArenaVoxelType voxelType);
}

#endif
