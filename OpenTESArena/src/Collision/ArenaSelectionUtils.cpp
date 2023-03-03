#include "ArenaSelectionUtils.h"

bool ArenaSelectionUtils::isVoxelSelectableAsPrimary(ArenaTypes::VoxelType voxelType)
{
	return (voxelType == ArenaTypes::VoxelType::Wall) ||
		(voxelType == ArenaTypes::VoxelType::Floor) ||
		(voxelType == ArenaTypes::VoxelType::Raised) ||
		(voxelType == ArenaTypes::VoxelType::Diagonal) ||
		(voxelType == ArenaTypes::VoxelType::TransparentWall) ||
		(voxelType == ArenaTypes::VoxelType::Edge);
}

bool ArenaSelectionUtils::isVoxelSelectableAsSecondary(ArenaTypes::VoxelType voxelType)
{
	return voxelType == ArenaTypes::VoxelType::Wall;
}
