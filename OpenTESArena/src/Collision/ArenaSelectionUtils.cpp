#include "ArenaSelectionUtils.h"

bool ArenaSelectionUtils::isVoxelSelectableAsPrimary(ArenaVoxelType voxelType)
{
	return (voxelType == ArenaVoxelType::Wall) ||
		(voxelType == ArenaVoxelType::Floor) ||
		(voxelType == ArenaVoxelType::Raised) ||
		(voxelType == ArenaVoxelType::Diagonal) ||
		(voxelType == ArenaVoxelType::TransparentWall) ||
		(voxelType == ArenaVoxelType::Edge);
}

bool ArenaSelectionUtils::isVoxelSelectableAsSecondary(ArenaVoxelType voxelType)
{
	return voxelType == ArenaVoxelType::Wall;
}
