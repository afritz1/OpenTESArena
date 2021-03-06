#include <algorithm>

#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "../Entities/EntityManager.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

namespace
{
	// For iterating only the portion of a level that the chunk overlaps.
	void GetChunkWritingRanges(const LevelInt2 &levelOffset, SNInt levelWidth, int levelHeight, WEInt levelDepth,
		SNInt *outStartX, int *outStartY, WEInt *outStartZ, SNInt *outEndX, int *outEndY, WEInt *outEndZ)
	{
		*outStartX = levelOffset.x;
		*outEndX = std::min(*outStartX + Chunk::WIDTH, levelWidth);
		*outStartY = 0;
		*outEndY = levelHeight;
		*outStartZ = levelOffset.y;
		*outEndZ = std::min(*outStartZ + Chunk::DEPTH, levelDepth);
	}
}

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

Chunk *ChunkManager::tryGetChunk(const ChunkInt2 &coord)
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(coord);
	return chunkIndex.has_value() ? &this->getChunk(*chunkIndex) : nullptr;
}

const Chunk *ChunkManager::tryGetChunk(const ChunkInt2 &coord) const
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(coord);
	return chunkIndex.has_value() ? &this->getChunk(*chunkIndex) : nullptr;
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

void ChunkManager::recycleChunk(int index)
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
}

void ChunkManager::populateChunkVoxelDefsFromLevel(Chunk &chunk, const LevelInfoDefinition &levelInfoDefinition)
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
}

void ChunkManager::populateChunkVoxelsFromLevel(Chunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInt2 &levelOffset)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	GetChunkWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

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

				// Add one to account for the air voxel definition being ID 0.
				const Chunk::VoxelID correctedVoxelID = voxelID + 1;
				chunk.setVoxel(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, correctedVoxelID);
			}
		}
	}
}

void ChunkManager::populateChunkDecoratorsFromLevel(Chunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	GetChunkWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	auto isInChunk = [startX, endX, startY, endY, startZ, endZ](const LevelInt3 &position)
	{
		return (position.x >= startX) && (position.x < endX) && (position.y >= startY) && (position.y < endY) &&
			(position.z >= startZ) && (position.z < endZ);
	};

	auto makeChunkPosition = [startX, startY, startZ](const LevelInt3 &position)
	{
		return VoxelInt3(position.x - startX, position.y - startY, position.z - startZ);
	};

	// Add transitions.
	for (int i = 0; i < levelDefinition.getTransitionPlacementDefCount(); i++)
	{
		const LevelDefinition::TransitionPlacementDef &placementDef = levelDefinition.getTransitionPlacementDef(i);
		const TransitionDefinition &transitionDef = levelInfoDefinition.getTransitionDef(placementDef.id);
		
		std::optional<Chunk::TransitionID> transitionID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (isInChunk(position))
			{
				if (!transitionID.has_value())
				{
					transitionID = chunk.addTransition(TransitionDefinition(transitionDef));
				}

				const VoxelInt3 voxel = makeChunkPosition(position);
				chunk.addTransitionPosition(*transitionID, voxel);
			}
		}
	}

	// Add triggers.
	for (int i = 0; i < levelDefinition.getTriggerPlacementDefCount(); i++)
	{
		const LevelDefinition::TriggerPlacementDef &placementDef = levelDefinition.getTriggerPlacementDef(i);
		const TriggerDefinition &triggerDef = levelInfoDefinition.getTriggerDef(placementDef.id);
		
		std::optional<Chunk::TriggerID> triggerID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (isInChunk(position))
			{
				if (!triggerID.has_value())
				{
					triggerID = chunk.addTrigger(TriggerDefinition(triggerDef));
				}

				const VoxelInt3 voxel = makeChunkPosition(position);
				chunk.addTriggerPosition(*triggerID, voxel);
			}
		}
	}

	// Add locks.
	for (int i = 0; i < levelDefinition.getLockPlacementDefCount(); i++)
	{
		const LevelDefinition::LockPlacementDef &placementDef = levelDefinition.getLockPlacementDef(i);
		const LockDefinition &lockDef = levelInfoDefinition.getLockDef(placementDef.id);
		
		std::optional<Chunk::LockID> lockID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (isInChunk(position))
			{
				if (!lockID.has_value())
				{
					lockID = chunk.addLock(LockDefinition(lockDef));
				}

				const VoxelInt3 voxel = makeChunkPosition(position);
				chunk.addLockPosition(*lockID, voxel);
			}
		}
	}

	// Add building names.
	for (int i = 0; i < levelDefinition.getBuildingNamePlacementDefCount(); i++)
	{
		const LevelDefinition::BuildingNamePlacementDef &placementDef = levelDefinition.getBuildingNamePlacementDef(i);
		const std::string &buildingName = levelInfoDefinition.getBuildingName(placementDef.id);
		
		std::optional<Chunk::BuildingNameID> buildingNameID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (isInChunk(position))
			{
				if (!buildingNameID.has_value())
				{
					buildingNameID = chunk.addBuildingName(std::string(buildingName));
				}

				const VoxelInt3 voxel = makeChunkPosition(position);
				chunk.addBuildingNamePosition(*buildingNameID, voxel);
			}
		}
	}
}

bool ChunkManager::populateChunk(int index, const ChunkInt2 &coord, const std::optional<int> &activeLevelIndex,
	const MapDefinition &mapDefinition)
{
	Chunk &chunk = this->getChunk(index);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(*activeLevelIndex);
		chunk.init(coord, levelDefinition.getHeight());
		this->populateChunkVoxelDefsFromLevel(chunk, levelInfoDefinition);

		// @todo: populate chunk entirely from default empty chunk (fast copy).
		// - probably get from MapDefinition::Interior eventually.
		constexpr Chunk::VoxelID floorVoxelID = 2;
		const Chunk::VoxelID ceilingVoxelID = [&levelInfoDefinition]()
		{
			// Find ceiling voxel definition.
			for (int i = 0; i < levelInfoDefinition.getVoxelDefCount(); i++)
			{
				const VoxelDefinition &voxelDef = levelInfoDefinition.getVoxelDef(i);
				if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
				{
					// Add 1 to account for air voxel definition at ID 0.
					return i + 1;
				}
			}

			// No ceiling found, use air instead.
			return 0;
		}();

		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				chunk.setVoxel(x, 0, z, floorVoxelID);

				if (chunk.getHeight() > 2)
				{
					chunk.setVoxel(x, 2, z, ceilingVoxelID);
				}
			}
		}

		if (ChunkUtils::touchesLevelDimensions(coord, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = coord * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxelsFromLevel(chunk, levelDefinition, levelOffset);
			this->populateChunkDecoratorsFromLevel(chunk, levelDefinition, levelInfoDefinition, levelOffset);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
		chunk.init(coord, levelDefinition.getHeight());
		this->populateChunkVoxelDefsFromLevel(chunk, levelInfoDefinition);

		// @todo: chunks outside the level are wrapped but only have floor voxels.
		// - just need to wrap based on level definitions, not chunk dimensions. So a 96x96 city
		//   would start wrapping halfway through chunks (1, 0) and (0, 1).

		if (ChunkUtils::touchesLevelDimensions(coord, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = coord * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxelsFromLevel(chunk, levelDefinition, levelOffset);
			this->populateChunkDecoratorsFromLevel(chunk, levelDefinition, levelInfoDefinition, levelOffset);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		DebugAssert(!activeLevelIndex.has_value());
		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(coord);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(levelDefIndex);
		chunk.init(coord, levelDefinition.getHeight());
		this->populateChunkVoxelDefsFromLevel(chunk, levelInfoDefinition);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
		const LevelInt2 levelOffset = LevelInt2::Zero;
		this->populateChunkVoxelsFromLevel(chunk, levelDefinition, levelOffset);
		this->populateChunkDecoratorsFromLevel(chunk, levelDefinition, levelInfoDefinition, levelOffset);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
		return false;
	}

	return true;
}

void ChunkManager::update(double dt, const ChunkInt2 &centerChunk, const std::optional<int> &activeLevelIndex,
	const MapDefinition &mapDefinition, int chunkDistance, bool updateChunkStates, EntityManager &entityManager)
{
	this->centerChunk = centerChunk;

	// Free any out-of-range chunks.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const ChunkInt2 coord = chunkPtr->getCoord();
		if (!ChunkUtils::isWithinActiveRange(centerChunk, coord, chunkDistance))
		{
			this->recycleChunk(i);

			// Notify entity manager that the chunk is being recycled.
			entityManager.removeChunk(coord);
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
					continue;
				}

				// Notify the entity manager about the new chunk.
				entityManager.addChunk(coord);
			}
		}
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	if (updateChunkStates)
	{
		// Update each chunk so they can animate/destroy faded voxel instances, etc..
		for (int i = 0; i < static_cast<int>(this->activeChunks.size()) - 1; i++)
		{
			ChunkPtr &chunkPtr = this->activeChunks[i];
			chunkPtr->update(dt);
		}
	}
}
