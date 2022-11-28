#include "AutomapUiModel.h"
#include "AutomapUiView.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/ArenaWildUtils.h"
#include "../World/ChunkUtils.h"

Double2 AutomapUiModel::makeAutomapOffset(const VoxelInt2 &playerVoxel)
{
	// Offsets from the top-left corner of the automap texture. Always at least one full chunk because
	// the player is in the middle of the active chunks.
	const VoxelInt2 chunkOffset(
		AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM,
		AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM);
	const VoxelInt2 playerVoxelOffset(ChunkUtils::CHUNK_DIM - playerVoxel.y - 1, playerVoxel.x);

	// Convert to real since the automap scrolling is in vector space.
	const Double2 offsetReal = VoxelUtils::getVoxelCenter(chunkOffset + playerVoxelOffset);

	// Negate the offset so it's how much the automap is pushed. It's the vector opposite of the automap
	// origin to the player's position.
	return -offsetReal;
}

NewInt2 AutomapUiModel::makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth)
{
	return ArenaWildUtils::getCenteredWildOrigin(voxel);
}
