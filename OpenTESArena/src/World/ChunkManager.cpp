#include <algorithm>

#include "ChunkManager.h"
#include "ChunkUtils.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "../Assets/ArenaTypes.h"
#include "../Entities/EntityManager.h"
#include "../Entities/EntityType.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
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

	bool IsInChunkWritingRange(const LevelInt3 &position, SNInt startX, SNInt endX, int startY, int endY,
		WEInt startZ, WEInt endZ)
	{
		return (position.x >= startX) && (position.x < endX) && (position.y >= startY) && (position.y < endY) &&
			(position.z >= startZ) && (position.z < endZ);
	}

	VoxelInt3 MakeChunkVoxelFromLevel(const LevelInt3 &levelPosition, SNInt chunkStartX, int chunkStartY, WEInt chunkStartZ)
	{
		return VoxelInt3(
			levelPosition.x - chunkStartX,
			levelPosition.y - chunkStartY,
			levelPosition.z - chunkStartZ);
	}

	VoxelDouble3 MakeChunkPointFromLevel(const LevelDouble3 &levelPosition, SNInt chunkStartX, int chunkStartY, WEInt chunkStartZ)
	{
		return VoxelDouble3(
			levelPosition.x - static_cast<SNDouble>(chunkStartX),
			levelPosition.y - static_cast<double>(chunkStartY),
			levelPosition.z - static_cast<WEDouble>(chunkStartZ));
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

void ChunkManager::getAdjacentVoxelDefs(const CoordInt3 &coord, const VoxelDefinition **outNorth,
	const VoxelDefinition **outEast, const VoxelDefinition **outSouth, const VoxelDefinition **outWest)
{
	auto getAdjacentCoord = [&coord](const VoxelInt2 &direction)
	{
		const VoxelInt3 diff(direction.x, 0, direction.y);
		return ChunkUtils::recalculateCoord(coord.chunk, coord.voxel + diff);
	};

	auto tryWriteVoxelDef = [](const Chunk *chunkPtr, const VoxelInt3 &voxel, const VoxelDefinition **outDef)
	{
		if (chunkPtr != nullptr)
		{
			const Chunk::VoxelID voxelID = chunkPtr->getVoxel(voxel.x, voxel.y, voxel.z);
			*outDef = &chunkPtr->getVoxelDef(voxelID);
		}
		else
		{
			*outDef = nullptr;
		}
	};

	const CoordInt3 northCoord = getAdjacentCoord(VoxelUtils::North);
	const CoordInt3 eastCoord = getAdjacentCoord(VoxelUtils::East);
	const CoordInt3 southCoord = getAdjacentCoord(VoxelUtils::South);
	const CoordInt3 westCoord = getAdjacentCoord(VoxelUtils::West);
	const Chunk *northChunkPtr = this->tryGetChunk(northCoord.chunk);
	const Chunk *eastChunkPtr = this->tryGetChunk(eastCoord.chunk);
	const Chunk *southChunkPtr = this->tryGetChunk(southCoord.chunk);
	const Chunk *westChunkPtr = this->tryGetChunk(westCoord.chunk);
	tryWriteVoxelDef(northChunkPtr, northCoord.voxel, outNorth);
	tryWriteVoxelDef(eastChunkPtr, eastCoord.voxel, outEast);
	tryWriteVoxelDef(southChunkPtr, southCoord.voxel, outSouth);
	tryWriteVoxelDef(westChunkPtr, westCoord.voxel, outWest);
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

void ChunkManager::populateChunkVoxelDefs(Chunk &chunk, const LevelInfoDefinition &levelInfoDefinition)
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

void ChunkManager::populateChunkVoxels(Chunk &chunk, const LevelDefinition &levelDefinition,
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

void ChunkManager::populateChunkDecorators(Chunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	GetChunkWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Add transitions.
	for (int i = 0; i < levelDefinition.getTransitionPlacementDefCount(); i++)
	{
		const LevelDefinition::TransitionPlacementDef &placementDef = levelDefinition.getTransitionPlacementDef(i);
		const TransitionDefinition &transitionDef = levelInfoDefinition.getTransitionDef(placementDef.id);
		
		std::optional<Chunk::TransitionID> transitionID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (IsInChunkWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!transitionID.has_value())
				{
					transitionID = chunk.addTransition(TransitionDefinition(transitionDef));
				}

				const VoxelInt3 voxel = MakeChunkVoxelFromLevel(position, startX, startY, startZ);
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
			if (IsInChunkWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!triggerID.has_value())
				{
					triggerID = chunk.addTrigger(TriggerDefinition(triggerDef));
				}

				const VoxelInt3 voxel = MakeChunkVoxelFromLevel(position, startX, startY, startZ);
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
			if (IsInChunkWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!lockID.has_value())
				{
					lockID = chunk.addLock(LockDefinition(lockDef));
				}

				const VoxelInt3 voxel = MakeChunkVoxelFromLevel(position, startX, startY, startZ);
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
			if (IsInChunkWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!buildingNameID.has_value())
				{
					buildingNameID = chunk.addBuildingName(std::string(buildingName));
				}

				const VoxelInt3 voxel = MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addBuildingNamePosition(*buildingNameID, voxel);
			}
		}
	}
}

void ChunkManager::populateChunkVoxelInsts(Chunk &chunk)
{
	// @todo: only iterate over chunk writing ranges

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
				const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
				if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
				{
					const VoxelInt3 voxel(x, y, z);
					const CoordInt3 coord(chunk.getCoord(), voxel);
					DebugAssert(chunk.tryGetVoxelInst(voxel, VoxelInstance::Type::Chasm) == nullptr);

					const VoxelDefinition *northDef, *eastDef, *southDef, *westDef;
					this->getAdjacentVoxelDefs(coord, &northDef, &eastDef, &southDef, &westDef);

					const bool hasNorthFace = (northDef != nullptr) && northDef->allowsChasmFace();
					const bool hasEastFace = (eastDef != nullptr) && eastDef->allowsChasmFace();
					const bool hasSouthFace = (southDef != nullptr) && southDef->allowsChasmFace();
					const bool hasWestFace = (westDef != nullptr) && westDef->allowsChasmFace();
					if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
					{
						VoxelInstance voxelInst = VoxelInstance::makeChasm(
							x, y, z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
						chunk.addVoxelInst(std::move(voxelInst));
					}
				}
			}
		}
	}
}

void ChunkManager::populateChunkEntities(Chunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset, EntityManager &entityManager)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	GetChunkWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Cosmetic random (initial creature sound timing, etc.).
	Random random;

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID entityDefID = placementDef.id; // Equivalent to a EntityDefinitionLibrary EntityDefID as far as I'm aware?
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(entityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		for (const LevelDouble3 &position : placementDef.positions)
		{
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, levelInfoDefinition.getCeilingScale());
			if (IsInChunkWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				const VoxelDouble3 point = MakeChunkPointFromLevel(position, startX, startY, startZ);

				// @todo: put most of this in EntityGeneration.

				EntityRef entity = entityManager.makeEntity(entityType); // @todo: decide if chunk should be an argument too
				Entity *entityPtr = entity.get();

				EntityAnimationInstance animInst;
				animInst.init(animDef);
				// @todo: set anim inst to the correct state for the entity
				// @todo: let the entity acquire the anim inst instead of copying
				// @todo: set streetlight anim state if night lights are active

				if (entityType == EntityType::Static)
				{
					StaticEntity *staticEntity = dynamic_cast<StaticEntity*>(entityPtr);
					if (entityDefType == EntityDefinition::Type::StaticNPC)
					{
						staticEntity->initNPC(entityDefID, animInst);
					}
					else if (entityDefType == EntityDefinition::Type::Item)
					{
						// @todo: initialize as an item
						staticEntity->initDoodad(entityDefID, animInst);
						DebugLogError("Item entity initialization not implemented.");
					}
					else if (entityDefType == EntityDefinition::Type::Container)
					{
						staticEntity->initContainer(entityDefID, animInst);
					}
					else if (entityDefType == EntityDefinition::Type::Transition)
					{
						staticEntity->initTransition(entityDefID, animInst);
					}
					else if (entityDefType == EntityDefinition::Type::Doodad)
					{
						staticEntity->initDoodad(entityDefID, animInst);
					}
					else
					{
						DebugNotImplementedMsg(std::to_string(static_cast<int>(entityDefType)));
					}
				}
				else if (entityType == EntityType::Dynamic)
				{
					DynamicEntity *dynamicEntity = dynamic_cast<DynamicEntity*>(entityPtr);
					const VoxelDouble2 direction = CardinalDirection::North;
					const CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(direction);

					if (entityDefType == EntityDefinition::Type::Enemy)
					{
						dynamicEntity->initCreature(entityDefID, animInst, direction, random);
					}
					else
					{
						DebugNotImplementedMsg(std::to_string(static_cast<int>(entityDefType)));
					}
				}
				else
				{
					DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
				}

				DebugNotImplemented();
				// @todo: set entity position in chunk
			}
		}
	}
}

void ChunkManager::populateChunk(int index, const ChunkInt2 &coord, const std::optional<int> &activeLevelIndex,
	const MapDefinition &mapDefinition, EntityManager &entityManager)
{
	Chunk &chunk = this->getChunk(index);
	
	// Notify the entity manager about the new chunk so entities can be spawned in it.
	entityManager.addChunk(coord);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(*activeLevelIndex);
		chunk.init(coord, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

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
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkVoxelInsts(chunk);
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityManager);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
		chunk.init(coord, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// @todo: chunks outside the level are wrapped but only have floor voxels.
		// - just need to wrap based on level definitions, not chunk dimensions. So a 96x96 city
		//   would start wrapping halfway through chunks (1, 0) and (0, 1).

		if (ChunkUtils::touchesLevelDimensions(coord, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = coord * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkVoxelInsts(chunk);
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityManager);
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
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
		const LevelInt2 levelOffset = LevelInt2::Zero;
		this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
		this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
		this->populateChunkVoxelInsts(chunk);
		this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}
}

void ChunkManager::updateChunkPerimeter(Chunk &chunk)
{
	auto tryUpdateChasm = [this, &chunk](const VoxelInt3 &voxel)
	{
		const CoordInt3 coord(chunk.getCoord(), voxel);
		auto getChasmFaces = [this, &coord](bool *outNorth, bool *outEast, bool *outSouth, bool *outWest)
		{
			const VoxelDefinition *northDef, *eastDef, *southDef, *westDef;
			this->getAdjacentVoxelDefs(coord, &northDef, &eastDef, &southDef, &westDef);
			*outNorth = (northDef != nullptr) && northDef->allowsChasmFace();
			*outEast = (eastDef != nullptr) && eastDef->allowsChasmFace();
			*outSouth = (southDef != nullptr) && southDef->allowsChasmFace();
			*outWest = (westDef != nullptr) && westDef->allowsChasmFace();
		};
		
		constexpr VoxelInstance::Type voxelInstType = VoxelInstance::Type::Chasm;
		VoxelInstance *chasmInst = chunk.tryGetVoxelInst(voxel, voxelInstType);
		if (chasmInst != nullptr)
		{
			// The voxel instance already exists. See if it should be updated or removed.
			bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
			getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

			if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
			{
				// The voxel instance is still needed. Update its chasm walls.
				VoxelInstance::ChasmState &chasmState = chasmInst->getChasmState();
				chasmState.init(hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
			}
			else
			{
				// The voxel instance no longer has any interesting data.
				chunk.removeVoxelInst(voxel, voxelInstType);
			}
		}
		else
		{
			// No voxel instance yet. If it's a chasm, add a new voxel instance.
			const Chunk::VoxelID voxelID = chunk.getVoxel(voxel.x, voxel.y, voxel.z);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
			if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
				getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

				if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
				{
					VoxelInstance voxelInst = VoxelInstance::makeChasm(
						voxel.x, voxel.y, voxel.z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
					chunk.addVoxelInst(std::move(voxelInst));
				}
			}
		}
	};

	// North and south sides.
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			constexpr SNInt northX = 0;
			constexpr SNInt southX = Chunk::WIDTH - 1;
			tryUpdateChasm(VoxelInt3(northX, y, z));
			tryUpdateChasm(VoxelInt3(southX, y, z));
		}
	}

	// East and west sides, minus the corners.
	for (SNInt x = 1; x < (Chunk::WIDTH - 1); x++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			constexpr WEInt eastZ = 0;
			constexpr WEInt westZ = Chunk::DEPTH - 1;
			tryUpdateChasm(VoxelInt3(x, y, eastZ));
			tryUpdateChasm(VoxelInt3(x, y, westZ));
		}
	}
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
				this->populateChunk(spawnIndex, coord, activeLevelIndex, mapDefinition, entityManager);
			}
		}
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	const int activeChunkCount = static_cast<int>(this->activeChunks.size());
	if (updateChunkStates)
	{
		// Update each chunk so they can animate/destroy faded voxel instances, etc..
		for (int i = 0; i < activeChunkCount; i++)
		{
			ChunkPtr &chunkPtr = this->activeChunks[i];
			chunkPtr->update(dt);
		}
	}

	// Update chunk perimeters in case voxels on the edge of one chunk affect context-sensitive
	// voxels in adjacent chunks. It might be a little inefficient to do this every frame, but
	// it easily handles cases of modified voxels on a chunk edge, and it is only updating chunk
	// edges, so it should be very fast.
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->updateChunkPerimeter(*chunkPtr);
	}
}
