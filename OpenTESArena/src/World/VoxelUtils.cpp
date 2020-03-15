#include "VoxelUtils.h"

#include "components/debug/Debug.h"

NewInt2 VoxelUtils::originalVoxelToNewVoxel(const OriginalInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	// These have a -1 since all .MIF start points are in the center of a voxel, giving a minimum
	// distance of 0.5 from grid sides, thus guaranteeing that no out-of-bounds grid access will
	// occur in those cases. This one doesn't have that bias (due to integers), and as such, values
	// may go out of grid range if using the unmodified dimensions.
	return NewInt2((gridWidth - 1) - voxel.y, (gridDepth - 1) - voxel.x);
}

OriginalInt2 VoxelUtils::newVoxelToOriginalVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	return VoxelUtils::originalVoxelToNewVoxel(voxel, gridWidth, gridDepth);
}

Double2 VoxelUtils::getTransformedVoxel(const Double2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	return Double2(
		static_cast<double>(gridWidth) - voxel.y,
		static_cast<double>(gridDepth) - voxel.x);
}

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
	NSInt gridWidth, EWInt gridDepth)
{
	DebugNotImplemented();
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel = voxel + (chunk * CHUNK_DIM);
	return NewInt2((gridWidth - 1) - absoluteChunkVoxel.y, absoluteChunkVoxel.x);
}

ChunkCoord VoxelUtils::newVoxelToChunkVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	// @todo: probably need to include "next higher multiple of 64" for newVoxel.
	// - i.e. if gridWidth (NSInt) is 80, it's in chunk 0,1.

	// Can assume that the chunk space will always be equal to or greater than the new voxel space
	// (i.e. if the level is 80x80 voxels, the chunk grid would be 2x2).

	DebugNotImplemented();
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel((gridWidth - 1) - voxel.y, voxel.x); // <<< currently wrong! newVoxel +Z 5 might be absoluteChunkVoxel +X 40 or something.
	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(absoluteChunkVoxel.x / CHUNK_DIM, absoluteChunkVoxel.y / CHUNK_DIM);
	chunkCoord.voxel = ChunkVoxelInt2(absoluteChunkVoxel.x % CHUNK_DIM, absoluteChunkVoxel.y % CHUNK_DIM);
	return chunkCoord;
}

ChunkInt2 VoxelUtils::newVoxelToChunk(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	DebugNotImplemented();
}
