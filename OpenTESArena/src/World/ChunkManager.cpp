#include <optional>

#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "WorldType.h"
#include "../Entities/EntityManager.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

ChunkManager::ChunkManager()
{
	this->worldType = static_cast<WorldType>(-1);
	this->chunkDistance = -1;
}

void ChunkManager::init(WorldType worldType, int chunkDistance)
{
	DebugAssertMsg(this->activeChunks.empty(), "Expected no active chunks.");

	SNInt chunkCountX;
	WEInt chunkCountZ;
	ChunkUtils::getPotentiallyVisibleChunkCounts(chunkDistance, &chunkCountX, &chunkCountZ);

	const int totalChunkCount = chunkCountX * chunkCountZ;
	this->chunkPool = std::vector<ChunkPtr>(totalChunkCount);

	// Initialize chunk pool chunks to empty.
	for (ChunkPtr &chunkPtr : this->chunkPool)
	{
		chunkPtr = std::make_unique<Chunk>();
	}

	this->worldType = worldType;
	this->chunkDistance = chunkDistance;
}

bool ChunkManager::isValidChunkID(ChunkID id) const
{
	return (id >= 0) && (id < static_cast<int>(this->activeChunks.size()));
}

ChunkManager::ChunkPtr &ChunkManager::getChunkPtr(ChunkID id)
{
	DebugAssertIndex(this->activeChunks, id);
	return this->activeChunks[id];
}

const ChunkManager::ChunkPtr &ChunkManager::getChunkPtr(ChunkID id) const
{
	DebugAssertIndex(this->activeChunks, id);
	return this->activeChunks[id];
}

int ChunkManager::getChunkCount() const
{
	return static_cast<int>(this->activeChunks.size());
}

ChunkID ChunkManager::getChunkID(int index) const
{
	DebugAssertIndex(this->activeChunks, index);
	return static_cast<ChunkID>(index);
}

bool ChunkManager::tryGetChunkID(const ChunkInt2 &coord, ChunkID *outID) const
{
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		if (chunkPtr->getCoord() == coord)
		{
			*outID = this->getChunkID(i);
			return true;
		}
	}

	return false;
}

Chunk &ChunkManager::getChunk(ChunkID id)
{
	return *this->getChunkPtr(id);
}

const Chunk &ChunkManager::getChunk(ChunkID id) const
{
	return *this->getChunkPtr(id);
}

ChunkID ChunkManager::spawnChunk()
{
	DebugAssertMsg(this->chunkPool.size() > 0, "No more chunks allocated.");
	ChunkPtr chunkPtr = std::move(this->chunkPool.back());
	this->chunkPool.pop_back();

	// Find open spot in active chunks list, or append new slot.
	std::optional<int> existingIndex;
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &activeChunkPtr = this->activeChunks[i];
		if (activeChunkPtr == nullptr)
		{
			existingIndex = i;
			break;
		}
	}

	ChunkID id;
	if (existingIndex.has_value())
	{
		this->activeChunks[*existingIndex] = std::move(chunkPtr);
		id = this->getChunkID(*existingIndex);
	}
	else
	{
		this->activeChunks.emplace_back(std::move(chunkPtr));
		id = this->getChunkID(static_cast<int>(this->activeChunks.size()) - 1);
	}

	return id;
}

void ChunkManager::recycleChunk(ChunkID id, EntityManager &entityManager)
{
	DebugAssert(this->isValidChunkID(id));
	ChunkPtr &chunkPtr = this->getChunkPtr(id);
	const ChunkInt2 chunkCoord = chunkPtr->getCoord();

	// @todo: save chunk changes

	// Move chunk back to chunk pool, leaving active chunk slot null.
	chunkPtr->clear();
	this->chunkPool.push_back(std::move(chunkPtr));

	// Clear entities in the chunk (mark them for delete or however makes sense).
	entityManager.clearChunk(chunkCoord);
}

bool ChunkManager::populateChunk(ChunkID id, WorldType worldType, EntityManager &entityManager)
{
	DebugAssert(this->isValidChunkID(id));
	Chunk &chunk = this->getChunk(id);

	if (worldType == WorldType::Interior)
	{
		// @todo
		// Needs to know if the chunk coordinate intersects the level dimensions, so it knows
		// to grab voxel data from the level definition. Otherwise, default empty chunk.
	}
	else if (worldType == WorldType::City)
	{
		// @todo
		// Same as interior, except chunks outside the level are wrapped with only floor voxels.
	}
	else if (worldType == WorldType::Wilderness)
	{
		// @todo
		// Get the .RMD file (or equivalent) that goes in that chunk's spot.
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(worldType)));
		return false;
	}

	return true;
}

void ChunkManager::update(const ChunkInt2 &playerChunk, WorldType worldType,
	EntityManager &entityManager)
{
	// Free out-of-range chunks.
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		if (chunkPtr != nullptr)
		{
			const ChunkInt2 &coord = chunkPtr->getCoord();
			const bool shouldRemainActive = ChunkUtils::isWithinActiveRange(
				playerChunk, coord, this->chunkDistance);

			if (!shouldRemainActive)
			{
				const ChunkID chunkID = this->getChunkID(i);
				this->recycleChunk(chunkID, entityManager);
			}
		}
	}

	// Add new chunks to take the place of the removed ones. Get all the surrounding chunk coords
	// for the player chunk and see which ones aren't in active list.
	ChunkInt2 minCoord, maxCoord;
	ChunkUtils::getSurroundingChunks(playerChunk, this->chunkDistance, &minCoord, &maxCoord);

	for (WEInt y = minCoord.y; y < maxCoord.y; y++)
	{
		for (SNInt x = minCoord.x; x < maxCoord.x; x++)
		{
			const ChunkInt2 coord(x, y);			
			ChunkID chunkID;
			if (!this->tryGetChunkID(coord, &chunkID))
			{
				chunkID = this->spawnChunk();
				this->populateChunk(chunkID, this->worldType, entityManager);
			}
		}
	}
}
