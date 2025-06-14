#ifndef SPECIALIZED_CHUNK_MANAGER_H
#define SPECIALIZED_CHUNK_MANAGER_H

#include <algorithm>
#include <memory>
#include <vector>

#include "Chunk.h"

#include "components/debug/Debug.h"

// Inherited by any chunk managers that implement an engine system using chunks (voxels, entities, etc.).
template<typename ChunkType>
class SpecializedChunkManager
{
protected:
	static_assert(std::is_base_of_v<Chunk, ChunkType>);

	using ChunkPtr = std::unique_ptr<ChunkType>;

	template<typename VoxelIdType>
	using VoxelIdFunc = VoxelIdType(*)(const ChunkType &chunk, const VoxelInt3 &voxel);

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;

	int findChunkIndex(const ChunkInt2 &position) const
	{
		const int chunkCount = static_cast<int>(this->activeChunks.size());
		for (int i = 0; i < chunkCount; i++)
		{
			const ChunkPtr &chunkPtr = this->activeChunks[i];
			if (chunkPtr->position == position)
			{
				return i;
			}
		}

		return -1;
	}

	int getChunkIndex(const ChunkInt2 &position) const
	{
		const int index = this->findChunkIndex(position);
		if (index < 0)
		{
			DebugLogErrorFormat("Chunk (%s) not found.", position.toString().c_str());
		}

		return index;
	}

	// Gets the def IDs adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	template<typename VoxelIdType>
	void getAdjacentVoxelIDsInternal(const CoordInt3 &coord, VoxelIdFunc<VoxelIdType> voxelIdFunc, VoxelIdType defaultID,
		int *outNorthChunkIndex, int *outEastChunkIndex, int *outSouthChunkIndex, int *outWestChunkIndex,
		VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID, VoxelIdType *outWestID)
	{
		const CoordInt3 adjacentCoords[] =
		{
			VoxelUtils::getCoordWithOffset(coord, VoxelUtils::North),
			VoxelUtils::getCoordWithOffset(coord, VoxelUtils::East),
			VoxelUtils::getCoordWithOffset(coord, VoxelUtils::South),
			VoxelUtils::getCoordWithOffset(coord, VoxelUtils::West)
		};

		int *outAdjacentChunkIndices[] =
		{
			outNorthChunkIndex,
			outEastChunkIndex,
			outSouthChunkIndex,
			outWestChunkIndex
		};

		VoxelIdType *outAdjacentVoxelIDs[] =
		{
			outNorthID,
			outEastID,
			outSouthID,
			outWestID
		};

		ChunkInt2 cachedChunkPositions[4];
		int cachedChunkIndices[4];
		int cachedChunkPositionCount = 0;

		// Reuse chunk index lookups as they get expensive with large view distance.
		for (int i = 0; i < static_cast<int>(std::size(adjacentCoords)); i++)
		{
			const CoordInt3 adjacentCoord = adjacentCoords[i];
			const ChunkInt2 adjacentChunkPos = adjacentCoord.chunk;
			int &adjacentChunkIndex = *outAdjacentChunkIndices[i];

			bool foundExistingChunkIndex = false;
			for (int cacheIndex = 0; cacheIndex < cachedChunkPositionCount; cacheIndex++)
			{
				if (adjacentChunkPos == cachedChunkPositions[cacheIndex])
				{
					adjacentChunkIndex = cachedChunkIndices[cacheIndex];
					foundExistingChunkIndex = true;
					break;
				}
			}

			if (!foundExistingChunkIndex)
			{
				adjacentChunkIndex = this->findChunkIndex(adjacentChunkPos);
				if (adjacentChunkIndex >= 0)
				{
					DebugAssertIndex(cachedChunkPositions, cachedChunkPositionCount);
					cachedChunkPositions[cachedChunkPositionCount] = adjacentChunkPos;
					cachedChunkIndices[cachedChunkPositionCount] = adjacentChunkIndex;
					cachedChunkPositionCount++;
				}
			}

			if (adjacentChunkIndex >= 0)
			{
				const ChunkType &adjacentChunk = this->getChunkAtIndex(adjacentChunkIndex);
				const VoxelInt3 adjacentVoxel = adjacentCoord.voxel;
				*outAdjacentVoxelIDs[i] = voxelIdFunc(adjacentChunk, adjacentVoxel);
			}
			else
			{
				*outAdjacentVoxelIDs[i] = defaultID;
			}
		}
	}

	// Takes a chunk from the chunk pool, moves it to the active chunks, and returns its index.
	int spawnChunk()
	{
		if (!this->chunkPool.empty())
		{
			this->activeChunks.emplace_back(std::move(this->chunkPool.back()));
			this->chunkPool.pop_back();
		}
		else
		{
			// Always allow expanding in the event that chunk distance is increased.
			this->activeChunks.emplace_back(std::make_unique<ChunkType>());
		}

		return static_cast<int>(this->activeChunks.size()) - 1;
	}

	// Clears the chunk and removes it from the active chunks.
	void recycleChunk(int index)
	{
		DebugAssertIndex(this->activeChunks, index);
		ChunkPtr &chunkPtr = this->activeChunks[index];
		const ChunkInt2 chunkPos = chunkPtr->position;

		// @todo: save chunk changes

		// Move chunk to chunk pool. It's okay to shift chunk pointers around because this is during the 
		// time when references get invalidated.
		chunkPtr->clear();
		this->chunkPool.emplace_back(std::move(chunkPtr));
		this->activeChunks.erase(this->activeChunks.begin() + index);
	}
public:
	int getChunkCount() const
	{
		return static_cast<int>(this->activeChunks.size());
	}

	ChunkType &getChunkAtIndex(int index)
	{
		DebugAssertIndex(this->activeChunks, index);
		return *this->activeChunks[index];
	}

	const ChunkType &getChunkAtIndex(int index) const
	{
		DebugAssertIndex(this->activeChunks, index);
		return *this->activeChunks[index];
	}

	ChunkType *findChunkAtPosition(const ChunkInt2 &position)
	{
		const int index = this->findChunkIndex(position);
		if (index < 0)
		{
			return nullptr;
		}

		return &this->getChunkAtIndex(index);
	}

	const ChunkType *findChunkAtPosition(const ChunkInt2 &position) const
	{
		const int index = this->findChunkIndex(position);
		if (index < 0)
		{
			return nullptr;
		}

		return &this->getChunkAtIndex(index);
	}

	ChunkType &getChunkAtPosition(const ChunkInt2 &position)
	{
		const int index = this->getChunkIndex(position);
		return this->getChunkAtIndex(index);
	}

	const ChunkType &getChunkAtPosition(const ChunkInt2 &position) const
	{
		const int index = this->getChunkIndex(position);
		return this->getChunkAtIndex(index);
	}

	void recycleAllChunks()
	{
		for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
		{
			this->recycleChunk(i);
		}
	}
};

#endif
