#include <algorithm>

#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "WorldType.h"
#include "../Entities/EntityManager.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

int ChunkManager::getChunkCount() const
{
	return static_cast<int>(this->activeChunks.size());
}

Chunk &ChunkManager::getChunk(int index)
{
	DebugAssertIndex(this->activeChunks, index);
	const ChunkPtr &chunkPtr = this->activeChunks[index];

	DebugAssert(chunkPtr != nullptr);
	return *chunkPtr;
}

const Chunk &ChunkManager::getChunk(int index) const
{
	DebugAssertIndex(this->activeChunks, index);
	const ChunkPtr &chunkPtr = this->activeChunks[index];

	DebugAssert(chunkPtr != nullptr);
	return *chunkPtr;
}

std::optional<int> ChunkManager::tryGetChunkIndex(const ChunkInt2 &coord) const
{
	const auto iter = std::find_if(this->activeChunks.begin(), this->activeChunks.end(),
		[&coord](const ChunkPtr &chunkPtr)
	{
		return chunkPtr->getCoord() == coord;
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

int ChunkManager::getCenterChunkIndex() const
{
	const std::optional<int> index = this->tryGetChunkIndex(this->centerChunk);
	DebugAssert(index.has_value());
	return *index;
}

int ChunkManager::spawnChunk()
{
	if (!this->chunkPool.empty())
	{
		this->activeChunks.emplace_back(std::move(this->chunkPool.back()));
		this->chunkPool.pop_back();
	}
	else
	{
		// Always allow expanding in the event that chunk distance is increased.
		this->activeChunks.emplace_back(std::make_unique<Chunk>());
	}

	return static_cast<int>(this->activeChunks.size()) - 1;
}

void ChunkManager::recycleChunk(int index, EntityManager &entityManager)
{
	DebugAssertIndex(this->activeChunks, index);
	ChunkPtr &chunkPtr = this->activeChunks[index];
	const ChunkInt2 coord = chunkPtr->getCoord();

	// @todo: save chunk changes

	// Move chunk to chunk pool. It's okay to shift chunk pointers around because this is during the 
	// time when references get invalidated.
	chunkPtr->clear();
	this->chunkPool.emplace_back(std::move(chunkPtr));
	this->activeChunks.erase(this->activeChunks.begin() + index);

	// Notify entity manager that the chunk is being cleared.
	entityManager.clearChunk(coord);
}

bool ChunkManager::populateChunk(int index, const ChunkInt2 &coord, WorldType worldType,
	EntityManager &entityManager)
{
	Chunk &chunk = this->getChunk(index);
	chunk.setCoord(coord);

	if (worldType == WorldType::Interior)
	{
		// @todo
		// Needs to know if the chunk coordinate intersects the level dimensions, so it knows
		// to grab voxel data from the level definition. Otherwise, default empty chunk.

		// @todo: do something with MapDefinition::Interior
	}
	else if (worldType == WorldType::City)
	{
		// @todo
		// Same as interior, except chunks outside the level are wrapped with only floor voxels.
	}
	else if (worldType == WorldType::Wilderness)
	{
		// @todo
		// Get the .RMD file (or equivalent) that goes in this chunk's spot.

		// @todo: do something with MapDefinition::Wild
		//const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(worldType)));
		return false;
	}

	return true;
}

void ChunkManager::update(double dt, const ChunkInt2 &centerChunk, WorldType worldType,
	int chunkDistance, EntityManager &entityManager)
{
	this->centerChunk = centerChunk;

	// Free any out-of-range chunks.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const ChunkInt2 &coord = chunkPtr->getCoord();
		if (!ChunkUtils::isWithinActiveRange(centerChunk, coord, chunkDistance))
		{
			this->recycleChunk(i, entityManager);
		}
	}

	// Add new chunks until the area around the center chunk is filled.
	ChunkInt2 minCoord, maxCoord;
	ChunkUtils::getSurroundingChunks(centerChunk, chunkDistance, &minCoord, &maxCoord);

	for (WEInt y = minCoord.y; y <= maxCoord.y; y++)
	{
		for (SNInt x = minCoord.x; x <= maxCoord.x; x++)
		{
			const ChunkInt2 coord(x, y);
			const std::optional<int> index = this->tryGetChunkIndex(coord);
			if (!index.has_value())
			{
				const int spawnIndex = this->spawnChunk();
				if (!this->populateChunk(spawnIndex, coord, worldType, entityManager))
				{
					DebugLogError("Couldn't populate chunk \"" + std::to_string(spawnIndex) +
						"\" at (" + coord.toString() + ").");
				}
			}
		}
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	// Update each chunk so they can animate/destroy faded voxel instances, etc..
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()) - 1; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->update(dt);
	}
}
