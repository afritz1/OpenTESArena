#ifndef CHUNK_H
#define CHUNK_H

#include "ChunkUtils.h"
#include "Coord.h"
#include "../Math/MathUtils.h"

#include "components/utilities/BufferView3D.h"

// Base type for all chunks in the game world occupying 64x64 voxels.
struct Chunk
{
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	ChunkInt2 position;
	int height;

	bool isValidVoxel(SNInt x, int y, WEInt z) const;
protected:
	// To be called by derived chunk type.
	void init(const ChunkInt2 &position, int height);
	void clear();

	template<typename VoxelIdType>
	void getAdjacentIDsInternal(const VoxelInt3 &voxel, BufferView3D<const VoxelIdType> voxelIDs, VoxelIdType defaultID,
		VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID, VoxelIdType *outWestID) const
	{
		auto getIdOrDefault = [this, voxelIDs, defaultID](const VoxelInt3 &voxel)
		{
			if (!this->isValidVoxel(voxel.x, voxel.y, voxel.z))
			{
				return defaultID;
			}

			return voxelIDs.get(voxel.x, voxel.y, voxel.z);
		};

		const VoxelInt3 northVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::North);
		const VoxelInt3 eastVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::East);
		const VoxelInt3 southVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::South);
		const VoxelInt3 westVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::West);
		*outNorthID = getIdOrDefault(northVoxel);
		*outEastID = getIdOrDefault(eastVoxel);
		*outSouthID = getIdOrDefault(southVoxel);
		*outWestID = getIdOrDefault(westVoxel);
	}
};

#endif
