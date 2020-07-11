#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "WorldType.h"
#include "../Entities/EntityManager.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

namespace
{
	constexpr int NO_INDEX = -1;
}

void ChunkManager::init(int chunkDistance)
{
	DebugAssertMsg(this->activeChunks.empty(), "Expected no active chunks.");
	this->origin = ChunkInt2();

	SNInt chunkCountX;
	WEInt chunkCountZ;
	ChunkUtils::getPotentiallyVisibleChunkCounts(chunkDistance, &chunkCountX, &chunkCountZ);

	const int totalChunkCount = chunkCountX * chunkCountZ;
	this->chunkPool = std::vector<std::unique_ptr<Chunk>>(totalChunkCount);
}

int ChunkManager::findChunkIndex(const ChunkInt2 &coord) const
{
	const int activeChunkCount = static_cast<int>(this->activeChunks.size());
	for (int i = 0; i < activeChunkCount; i++)
	{
		const std::unique_ptr<Chunk> &chunk = this->activeChunks[i];
		if (chunk->getCoord() == coord)
		{
			return i;
		}
	}

	return NO_INDEX;
}

int ChunkManager::getChunkCount() const
{
	return static_cast<int>(this->activeChunks.size());
}

Chunk &ChunkManager::getChunkAtIndex(int index)
{
	DebugAssertIndex(this->activeChunks, index);
	return *this->activeChunks[index];
}

const Chunk &ChunkManager::getChunkAtIndex(int index) const
{
	DebugAssertIndex(this->activeChunks, index);
	return *this->activeChunks[index];
}

Chunk *ChunkManager::getChunk(const ChunkInt2 &coord)
{
	const int index = this->findChunkIndex(coord);
	return (index != NO_INDEX) ? &this->getChunkAtIndex(index) : nullptr;
}

const Chunk *ChunkManager::getChunk(const ChunkInt2 &coord) const
{
	const int index = this->findChunkIndex(coord);
	return (index != NO_INDEX) ? &this->getChunkAtIndex(index) : nullptr;
}

void ChunkManager::setOriginChunk(const ChunkInt2 &coord)
{
	this->origin = coord;

	// @todo: should this method do all the chunk freeing stuff behind the scenes as well, or
	// just set the origin chunk?

	// @todo: don't even need an origin, get rid of this method.
}

bool ChunkManager::tryPopulateChunk(const ChunkInt2 &coord, WorldType worldType, Game &game)
{
	std::unique_ptr<Chunk> &chunk = [this, &coord]() -> std::unique_ptr<Chunk>&
	{
		int index = this->findChunkIndex(coord);
		if (index == NO_INDEX)
		{
			// Grab from pool.
			DebugAssertMsg(this->chunkPool.size() > 0, "No chunks in pool for chunk (" + coord.toString() + ").");
			this->activeChunks.push_back(std::move(this->chunkPool.back()));
			this->chunkPool.pop_back();
			index = static_cast<int>(this->activeChunks.size()) - 1;
		}

		DebugAssertIndex(this->activeChunks, index);
		return this->activeChunks[index];
	}();

	// @todo: may need to allocate chunk in each worldType case if it is null.

	if (worldType == WorldType::City)
	{
		// @todo
		// Same as interior, except chunks outside the level are wrapped with only floor voxels.
	}
	else if (worldType == WorldType::Interior)
	{
		// @todo
		// Needs to know if the chunk coordinate intersects the level dimensions, so it knows
		// to grab voxel data from the level definition. Otherwise, default empty chunk.
	}
	else if (worldType == WorldType::Wilderness)
	{
		// @todo
		// Get the .RMD file (or equivalent) that goes in that chunk's spot.
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(worldType)));
	}

	return true;
}

bool ChunkManager::tryFreeChunk(const ChunkInt2 &coord, EntityManager &entityManager)
{
	const int index = this->findChunkIndex(coord);
	if (index == NO_INDEX)
	{
		DebugLogWarning("Couldn't find chunk (" + coord.toString() + ") to free.");
		return false;
	}

	DebugAssertIndex(this->activeChunks, index);
	std::unique_ptr<Chunk> &chunk = this->activeChunks[index];

	// Save chunk changes.
	// @todo

	// Move chunk back to chunk pool.
	chunk->clear();
	this->chunkPool.push_back(std::move(chunk));
	this->activeChunks.erase(this->activeChunks.begin() + index);

	// Clear entities in the chunk.
	// @todo: actually mark for delete here? Or can the entity manager do it internally?
	entityManager.clearChunk(coord);

	return true;
}

void ChunkManager::clear(EntityManager &entityManager)
{
	// Free all active chunks.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		const std::unique_ptr<Chunk> &chunk = this->activeChunks[i];
		if (!this->tryFreeChunk(chunk->getCoord(), entityManager))
		{
			DebugLogError("Couldn't free chunk (" + std::to_string(i) + ").");
		}
	}
}
