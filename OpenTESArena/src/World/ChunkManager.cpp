#include <algorithm>

#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "MapDefinition.h"
#include "MapType.h"
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

void ChunkManager::populateChunkFromLevel(Chunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset)
{
	// Add voxel definitions.
	for (int i = 0; i < levelInfoDefinition.getVoxelDefCount(); i++)
	{
		VoxelDefinition voxelDefinition = levelInfoDefinition.getVoxelDef(i);
		Chunk::VoxelID dummyID;
		if (!chunk.tryAddVoxelDef(std::move(voxelDefinition), &dummyID))
		{
			DebugLogError("Couldn't add voxel definition \"" + std::to_string(i) + "\" to chunk (voxel type \"" +
				std::to_string(static_cast<int>(voxelDefinition.type)) + "\".");
		}
	}

	// Iterate only the portion of the level that the chunk overlaps.
	const SNInt startX = levelOffset.x;
	const SNInt endX = std::min(startX + Chunk::WIDTH, levelDefinition.getWidth());
	const int startY = 0;
	const int endY = levelDefinition.getHeight();
	const WEInt startZ = levelOffset.y;
	const WEInt endZ = std::min(startZ + Chunk::DEPTH, levelDefinition.getDepth());

	// Set voxels.
	for (WEInt z = startZ; z < endZ; z++)
	{
		for (int y = startY; y < endY; y++)
		{
			for (SNInt x = startX; x < endX; x++)
			{
				const VoxelInt3 chunkVoxel(x - startX, y - startY, z - startZ);

				// Convert the voxel definition ID to a chunk voxel ID. If they don't match then the
				// chunk doesn't support that high of a voxel definition ID.
				const LevelDefinition::VoxelDefID voxelDefID = levelDefinition.getVoxel(x, y, z);
				const Chunk::VoxelID voxelID = static_cast<Chunk::VoxelID>(voxelDefID);
				if (static_cast<LevelDefinition::VoxelDefID>(voxelID) != voxelDefID)
				{
					continue;
				}

				chunk.setVoxel(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelID);
			}
		}
	}
}

bool ChunkManager::populateChunk(int index, const ChunkInt2 &coord, int activeLevelIndex,
	const MapDefinition &mapDefinition)
{
	Chunk &chunk = this->getChunk(index);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(activeLevelIndex);
		chunk.init(coord, levelDefinition.getHeight());

		// @todo: populate chunk entirely from default empty chunk (fast copy).
		// - probably get from MapDefinition::Interior eventually.

		if (ChunkUtils::touchesLevelDimensions(coord, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = coord * ChunkUtils::CHUNK_DIM;
			this->populateChunkFromLevel(chunk, levelDefinition, levelInfoDefinition, levelOffset);
		}
	}
	else if (mapType == MapType::City)
	{
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
		chunk.init(coord, levelDefinition.getHeight());

		// @todo: chunks outside the level are wrapped but only have floor voxels.
		// - just need to wrap based on level definitions, not chunk dimensions. So a 96x96 city
		//   would start wrapping halfway through chunks (1, 0) and (0, 1).

		if (ChunkUtils::touchesLevelDimensions(coord, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = coord * ChunkUtils::CHUNK_DIM;
			this->populateChunkFromLevel(chunk, levelDefinition, levelInfoDefinition, levelOffset);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(coord);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(levelDefIndex);
		chunk.init(coord, levelDefinition.getHeight());

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
		this->populateChunkFromLevel(chunk, levelDefinition, levelInfoDefinition, LevelInt2(0, 0));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
		return false;
	}

	return true;
}

void ChunkManager::update(double dt, const ChunkInt2 &centerChunk, int activeLevelIndex,
	const MapDefinition &mapDefinition, int chunkDistance, EntityManager &entityManager)
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
				if (!this->populateChunk(spawnIndex, coord, activeLevelIndex, mapDefinition))
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
