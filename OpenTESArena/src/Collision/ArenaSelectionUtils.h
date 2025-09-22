#ifndef ARENA_SELECTION_UTILS_H
#define ARENA_SELECTION_UTILS_H

#include "../Assets/ArenaTypes.h"
#include "../Assets/MIFUtils.h"

namespace ArenaSelectionUtils
{
	static constexpr int VOXEL_MAX_UNITS = 192;
	static constexpr double VOXEL_MAX_DISTANCE = static_cast<double>(VOXEL_MAX_UNITS) / MIFUtils::ARENA_UNITS;

	static constexpr int LOOT_MAX_UNITS = 170;
	static constexpr double LOOT_MAX_DISTANCE = static_cast<double>(LOOT_MAX_UNITS) / MIFUtils::ARENA_UNITS;

	static constexpr int CITIZEN_MAX_UNITS = 210; // Slightly higher than idle distance.
	static constexpr double CITIZEN_MAX_DISTANCE = static_cast<double>(CITIZEN_MAX_UNITS) / MIFUtils::ARENA_UNITS;

	// Can the given voxel type be selected with a left click?
	bool isVoxelSelectableAsPrimary(ArenaVoxelType voxelType);

	// Can the given voxel type be selected with a right click?
	bool isVoxelSelectableAsSecondary(ArenaVoxelType voxelType);
}

#endif
