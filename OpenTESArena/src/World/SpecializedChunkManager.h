#ifndef SPECIALIZED_CHUNK_MANAGER_H
#define SPECIALIZED_CHUNK_MANAGER_H

#include <algorithm>
#include <memory>
#include <optional>
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

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;

	template<typename VoxelIdType>
	using VoxelIdFunc = VoxelIdType(*)(const ChunkType &chunk, const VoxelInt3 &voxel);

	std::optional<int> tryGetChunkIndex(const ChunkInt2 &position) const
	{
		const auto iter = std::find_if(this->activeChunks.begin(), this->activeChunks.end(),
			[&position](const ChunkPtr &chunkPtr)
		{
			return chunkPtr->position == position;
		});

		if (iter != this->activeChunks.end())
		{
			return static_cast<int>(std::distance(this->activeChunks.begin(), iter));
		}
		else
		{
			return std::nullopt;
		}
	}

	int getChunkIndex(const ChunkInt2 &position) const
	{
		const std::optional<int> index = this->tryGetChunkIndex(position);

		// If this fails, we didn't properly update from the base chunk manager.
		DebugAssertMsg(index.has_value(), "Chunk (" + position.toString() + ") not found.");

		return *index;
	}

	// Gets the def IDs adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	template<typename VoxelIdType>
	void getAdjacentVoxelIDsInternal(const CoordInt3 &coord, VoxelIdFunc<VoxelIdType> voxelIdFunc, VoxelIdType defaultID,
		std::optional<int> *outNorthChunkIndex, std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex,
		std::optional<int> *outWestChunkIndex, VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID,
		VoxelIdType *outWestID)
	{
		const CoordInt3 adjacentCoords[] =
		{
			VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::North),
			VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::East),
			VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::South),
			VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::West)
		};

		std::optional<int> *outAdjacentChunkIndices[] =
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
			std::optional<int> &adjacentChunkIndex = *outAdjacentChunkIndices[i];

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
				adjacentChunkIndex = this->tryGetChunkIndex(adjacentChunkPos);
				if (adjacentChunkIndex.has_value())
				{
					DebugAssertIndex(cachedChunkPositions, cachedChunkPositionCount);
					cachedChunkPositions[cachedChunkPositionCount] = adjacentChunkPos;
					cachedChunkIndices[cachedChunkPositionCount] = *adjacentChunkIndex;
					cachedChunkPositionCount++;
				}
			}

			if (adjacentChunkIndex.has_value())
			{
				const ChunkType &adjacentChunk = this->getChunkAtIndex(*adjacentChunkIndex);
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

	ChunkType *tryGetChunkAtPosition(const ChunkInt2 &position)
	{
		const std::optional<int> index = this->tryGetChunkIndex(position);
		return index.has_value() ? &this->getChunkAtIndex(*index) : nullptr;
	}

	const ChunkType *tryGetChunkAtPosition(const ChunkInt2 &position) const
	{
		const std::optional<int> index = this->tryGetChunkIndex(position);
		return index.has_value() ? &this->getChunkAtIndex(*index) : nullptr;
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
