#include <algorithm>
#include <unordered_map>

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
	VoxelChunk::VoxelMeshDefID LevelVoxelMeshDefIdToChunkVoxelMeshDefID(LevelDefinition::VoxelMeshDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelChunk::VoxelMeshDefID>(levelVoxelDefID + 1);
	}

	VoxelChunk::VoxelTextureDefID LevelVoxelTextureDefIdToChunkVoxelTextureDefID(LevelDefinition::VoxelTextureDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelChunk::VoxelTextureDefID>(levelVoxelDefID + 1);
	}

	VoxelChunk::VoxelTraitsDefID LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(LevelDefinition::VoxelTraitsDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelChunk::VoxelTraitsDefID>(levelVoxelDefID + 1);
	}
}

int ChunkManager::getChunkCount() const
{
	return static_cast<int>(this->activeChunks.size());
}

VoxelChunk &ChunkManager::getChunk(int index)
{
	DebugAssertIndex(this->activeChunks, index);
	const VoxelChunkPtr &chunkPtr = this->activeChunks[index];

	DebugAssert(chunkPtr != nullptr);
	return *chunkPtr;
}

const VoxelChunk &ChunkManager::getChunk(int index) const
{
	DebugAssertIndex(this->activeChunks, index);
	const VoxelChunkPtr &chunkPtr = this->activeChunks[index];

	DebugAssert(chunkPtr != nullptr);
	return *chunkPtr;
}

std::optional<int> ChunkManager::tryGetChunkIndex(const ChunkInt2 &position) const
{
	const auto iter = std::find_if(this->activeChunks.begin(), this->activeChunks.end(),
		[&position](const VoxelChunkPtr &chunkPtr)
	{
		return chunkPtr->getPosition() == position;
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

VoxelChunk *ChunkManager::tryGetChunk(const ChunkInt2 &position)
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(position);
	return chunkIndex.has_value() ? &this->getChunk(*chunkIndex) : nullptr;
}

const VoxelChunk *ChunkManager::tryGetChunk(const ChunkInt2 &position) const
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(position);
	return chunkIndex.has_value() ? &this->getChunk(*chunkIndex) : nullptr;
}

int ChunkManager::getCenterChunkIndex() const
{
	const std::optional<int> index = this->tryGetChunkIndex(this->centerChunkPos);
	DebugAssert(index.has_value());
	return *index;
}

template <typename VoxelIdType>
void ChunkManager::getAdjacentVoxelIDsInternal(const CoordInt3 &coord, VoxelIdFunc<VoxelIdType> voxelIdFunc,
	VoxelIdType defaultID, std::optional<int> *outNorthChunkIndex, std::optional<int> *outEastChunkIndex,
	std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex, VoxelIdType *outNorthID,
	VoxelIdType *outEastID, VoxelIdType *outSouthID, VoxelIdType *outWestID)
{
	auto getIdOrDefault = [this, &voxelIdFunc, defaultID](const std::optional<int> &chunkIndex, const VoxelInt3 &voxel)
	{
		if (chunkIndex.has_value())
		{
			const VoxelChunk &chunk = this->getChunk(*chunkIndex);
			return voxelIdFunc(chunk, voxel);
		}
		else
		{
			return defaultID;
		}
	};

	const CoordInt3 northCoord = VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::North);
	const CoordInt3 eastCoord = VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::East);
	const CoordInt3 southCoord = VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::South);
	const CoordInt3 westCoord = VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::West);
	*outNorthChunkIndex = this->tryGetChunkIndex(northCoord.chunk);
	*outEastChunkIndex = this->tryGetChunkIndex(eastCoord.chunk);
	*outSouthChunkIndex = this->tryGetChunkIndex(southCoord.chunk);
	*outWestChunkIndex = this->tryGetChunkIndex(westCoord.chunk);
	*outNorthID = getIdOrDefault(*outNorthChunkIndex, northCoord.voxel);
	*outEastID = getIdOrDefault(*outEastChunkIndex, eastCoord.voxel);
	*outSouthID = getIdOrDefault(*outSouthChunkIndex, southCoord.voxel);
	*outWestID = getIdOrDefault(*outWestChunkIndex, westCoord.voxel);
}

void ChunkManager::getAdjacentVoxelMeshDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelMeshDefID *outNorthID, VoxelChunk::VoxelMeshDefID *outEastID, VoxelChunk::VoxelMeshDefID *outSouthID,
	VoxelChunk::VoxelMeshDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getVoxelMeshDefID(voxel.x, voxel.y, voxel.z);
	};

	ChunkManager::getAdjacentVoxelIDsInternal<VoxelChunk::VoxelMeshDefID>(coord, voxelIdFunc, VoxelChunk::AIR_VOXEL_MESH_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void ChunkManager::getAdjacentVoxelTextureDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelTextureDefID *outNorthID, VoxelChunk::VoxelTextureDefID *outEastID, VoxelChunk::VoxelTextureDefID *outSouthID,
	VoxelChunk::VoxelTextureDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getVoxelTextureDefID(voxel.x, voxel.y, voxel.z);
	};

	ChunkManager::getAdjacentVoxelIDsInternal<VoxelChunk::VoxelTextureDefID>(coord, voxelIdFunc, VoxelChunk::AIR_VOXEL_TEXTURE_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void ChunkManager::getAdjacentVoxelTraitsDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelTraitsDefID *outNorthID, VoxelChunk::VoxelTraitsDefID *outEastID, VoxelChunk::VoxelTraitsDefID *outSouthID,
	VoxelChunk::VoxelTraitsDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getVoxelTraitsDefID(voxel.x, voxel.y, voxel.z);
	};

	ChunkManager::getAdjacentVoxelIDsInternal<VoxelChunk::VoxelTraitsDefID>(coord, voxelIdFunc, VoxelChunk::AIR_VOXEL_TRAITS_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
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
		this->activeChunks.emplace_back(std::make_unique<VoxelChunk>());
	}

	return static_cast<int>(this->activeChunks.size()) - 1;
}

void ChunkManager::recycleChunk(int index)
{
	DebugAssertIndex(this->activeChunks, index);
	VoxelChunkPtr &chunkPtr = this->activeChunks[index];
	const ChunkInt2 chunkPos = chunkPtr->getPosition();

	// @todo: save chunk changes

	// Move chunk to chunk pool. It's okay to shift chunk pointers around because this is during the 
	// time when references get invalidated.
	chunkPtr->clear();
	this->chunkPool.emplace_back(std::move(chunkPtr));
	this->activeChunks.erase(this->activeChunks.begin() + index);
}

void ChunkManager::populateChunkVoxelDefs(VoxelChunk &chunk, const LevelInfoDefinition &levelInfoDefinition)
{
	for (int i = 0; i < levelInfoDefinition.getVoxelMeshDefCount(); i++)
	{
		VoxelMeshDefinition voxelMeshDef = levelInfoDefinition.getVoxelMeshDef(i);
		chunk.addVoxelMeshDef(std::move(voxelMeshDef));
	}

	for (int i = 0; i < levelInfoDefinition.getVoxelTextureDefCount(); i++)
	{
		VoxelTextureDefinition voxelTextureDef = levelInfoDefinition.getVoxelTextureDef(i);
		chunk.addVoxelTextureDef(std::move(voxelTextureDef));
	}

	for (int i = 0; i < levelInfoDefinition.getVoxelTraitsDefCount(); i++)
	{
		VoxelTraitsDefinition voxelTraitsDef = levelInfoDefinition.getVoxelTraitsDef(i);
		chunk.addVoxelTraitsDef(std::move(voxelTraitsDef));
	}
}

void ChunkManager::populateChunkVoxels(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInt2 &levelOffset)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Set voxels.
	for (WEInt z = startZ; z < endZ; z++)
	{
		for (int y = startY; y < endY; y++)
		{
			for (SNInt x = startX; x < endX; x++)
			{
				const VoxelInt3 chunkVoxel(x - startX, y - startY, z - startZ);
				const LevelDefinition::VoxelMeshDefID levelVoxelMeshDefID = levelDefinition.getVoxelMeshID(x, y, z);
				const LevelDefinition::VoxelTextureDefID levelVoxelTextureDefID = levelDefinition.getVoxelTextureID(x, y, z);
				const LevelDefinition::VoxelTraitsDefID levelVoxelTraitsDefID = levelDefinition.getVoxelTraitsID(x, y, z);
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = LevelVoxelMeshDefIdToChunkVoxelMeshDefID(levelVoxelMeshDefID);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setVoxelMeshDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelMeshDefID);
				chunk.setVoxelTextureDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTextureDefID);
				chunk.setVoxelTraitsDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTraitsDefID);
			}
		}
	}
}

void ChunkManager::populateChunkDecorators(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Add transitions.
	for (int i = 0; i < levelDefinition.getTransitionPlacementDefCount(); i++)
	{
		const LevelDefinition::TransitionPlacementDef &placementDef = levelDefinition.getTransitionPlacementDef(i);
		const TransitionDefinition &transitionDef = levelInfoDefinition.getTransitionDef(placementDef.id);
		
		std::optional<VoxelChunk::TransitionDefID> transitionDefID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!transitionDefID.has_value())
				{
					transitionDefID = chunk.addTransitionDef(TransitionDefinition(transitionDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addTransitionDefPosition(*transitionDefID, voxel);
			}
		}
	}

	// Add triggers.
	for (int i = 0; i < levelDefinition.getTriggerPlacementDefCount(); i++)
	{
		const LevelDefinition::TriggerPlacementDef &placementDef = levelDefinition.getTriggerPlacementDef(i);
		const VoxelTriggerDefinition &triggerDef = levelInfoDefinition.getTriggerDef(placementDef.id);
		
		std::optional<VoxelChunk::TriggerDefID> triggerDefID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!triggerDefID.has_value())
				{
					triggerDefID = chunk.addTriggerDef(VoxelTriggerDefinition(triggerDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addTriggerDefPosition(*triggerDefID, voxel);
			}
		}
	}

	// Add locks.
	for (int i = 0; i < levelDefinition.getLockPlacementDefCount(); i++)
	{
		const LevelDefinition::LockPlacementDef &placementDef = levelDefinition.getLockPlacementDef(i);
		const LockDefinition &lockDef = levelInfoDefinition.getLockDef(placementDef.id);
		
		std::optional<VoxelChunk::LockDefID> lockDefID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!lockDefID.has_value())
				{
					lockDefID = chunk.addLockDef(LockDefinition(lockDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addLockDefPosition(*lockDefID, voxel);
			}
		}
	}

	// Add building names (note that this doesn't apply to wilderness chunks because they can't rely on just the
	// level definition; they also need the chunk coordinate).
	for (int i = 0; i < levelDefinition.getBuildingNamePlacementDefCount(); i++)
	{
		const LevelDefinition::BuildingNamePlacementDef &placementDef = levelDefinition.getBuildingNamePlacementDef(i);
		const std::string &buildingName = levelInfoDefinition.getBuildingName(placementDef.id);
		
		std::optional<VoxelChunk::BuildingNameID> buildingNameID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!buildingNameID.has_value())
				{
					buildingNameID = chunk.addBuildingName(std::string(buildingName));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addBuildingNamePosition(*buildingNameID, voxel);
			}
		}
	}

	// Add door definitions.
	for (int i = 0; i < levelDefinition.getDoorPlacementDefCount(); i++)
	{
		const LevelDefinition::DoorPlacementDef &placementDef = levelDefinition.getDoorPlacementDef(i);
		const DoorDefinition &doorDef = levelInfoDefinition.getDoorDef(placementDef.id);

		std::optional<VoxelChunk::DoorDefID> doorDefID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!doorDefID.has_value())
				{
					doorDefID = chunk.addDoorDef(DoorDefinition(doorDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addDoorDefPosition(*doorDefID, voxel);
			}
		}
	}
	
	// Add chasm definitions.
	for (int i = 0; i < levelDefinition.getChasmPlacementDefCount(); i++)
	{
		const LevelDefinition::ChasmPlacementDef &placementDef = levelDefinition.getChasmPlacementDef(i);
		const ChasmDefinition &chasmDef = levelInfoDefinition.getChasmDef(placementDef.id);

		std::optional<VoxelChunk::ChasmDefID> chasmDefID;
		for (const LevelInt3 &position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!chasmDefID.has_value())
				{
					chasmDefID = chunk.addChasmDef(ChasmDefinition(chasmDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addChasmDefPosition(*chasmDefID, voxel);
			}
		}
	}
}

void ChunkManager::populateWildChunkBuildingNames(VoxelChunk &chunk,
	const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo, const LevelInfoDefinition &levelInfoDefinition)
{
	// Cache of level building names that have been added to the chunk.
	std::unordered_map<LevelDefinition::BuildingNameID, VoxelChunk::BuildingNameID> buildingNameIDs;

	for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < VoxelChunk::WIDTH; x++)
			{
				VoxelChunk::TransitionDefID transitionDefID;
				if (!chunk.tryGetTransitionDefID(x, y, z, &transitionDefID))
				{
					continue;
				}

				const TransitionDefinition &transitionDef = chunk.getTransitionDef(transitionDefID);
				if (transitionDef.getType() != TransitionType::EnterInterior)
				{
					continue;
				}

				const TransitionDefinition::InteriorEntranceDef &interiorEntranceDef = transitionDef.getInteriorEntrance();
				const MapGeneration::InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;
				const ArenaTypes::InteriorType interiorType = interiorGenInfo.getInteriorType();

				LevelDefinition::BuildingNameID buildingNameID;
				if (!buildingNameInfo.tryGetBuildingNameID(interiorType, &buildingNameID))
				{
					continue;
				}

				const std::string &buildingName = levelInfoDefinition.getBuildingName(buildingNameID);
				VoxelChunk::BuildingNameID chunkBuildingNameID;
				const auto nameIter = buildingNameIDs.find(buildingNameID);
				if (nameIter != buildingNameIDs.end())
				{
					chunkBuildingNameID = nameIter->second;
				}
				else
				{
					chunkBuildingNameID = chunk.addBuildingName(std::string(buildingName));
					buildingNameIDs.emplace(std::make_pair(buildingNameID, chunkBuildingNameID));
				}

				chunk.addBuildingNamePosition(chunkBuildingNameID, VoxelInt3(x, y, z));
			}
		}
	}
}

void ChunkManager::populateChunkVoxelInsts(VoxelChunk &chunk)
{
	// @todo: only iterate over chunk writing ranges

	DebugLogError("Not implemented: ChunkManager::populateChunkVoxelInsts()");

	for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < VoxelChunk::WIDTH; x++)
			{
				/*const VoxelChunk::VoxelID voxelID = chunk.getVoxelID(x, y, z);
				const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
				if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
				{
					const VoxelInt3 voxel(x, y, z);
					const CoordInt3 coord(chunk.getPosition(), voxel);
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
				}*/
			}
		}
	}
}

void ChunkManager::populateChunkEntities(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
	const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, EntityManager &entityManager)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Cosmetic random (initial creature sound timing, etc.).
	Random random;

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		std::optional<EntityDefID> entityDefID;
		for (const LevelDouble3 &position : placementDef.positions)
		{
			const double ceilingScale = levelInfoDefinition.getCeilingScale();
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, ceilingScale);
			if (ChunkUtils::IsInWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				if (!entityDefID.has_value())
				{
					entityDefID = entityManager.addEntityDef(EntityDefinition(entityDef), entityDefLibrary);
				}

				const VoxelDouble3 point = ChunkUtils::MakeChunkPointFromLevel(position, startX, startY, startZ);
				Entity *entity = EntityGeneration::makeEntity(entityType, entityDefType, *entityDefID,
					entityDef, animDef, entityGenInfo, random, entityManager);

				// Set entity position in chunk last. This has the potential to change the entity's chunk
				// and invalidate the local entity pointer.
				const CoordDouble2 coord(chunk.getPosition(), VoxelDouble2(point.x, point.z));
				entity->setPosition(coord, entityManager);
			}
		}
	}

	if (citizenGenInfo.has_value())
	{
		// Spawn citizens if the total active limit has not been reached.
		const int currentCitizenCount = CitizenUtils::getCitizenCount(entityManager);
		const int remainingCitizensToSpawn = std::min(
			CitizenUtils::MAX_ACTIVE_CITIZENS - currentCitizenCount, CitizenUtils::CITIZENS_PER_CHUNK);

		for (int i = 0; i < remainingCitizensToSpawn; i++)
		{
			if (!CitizenUtils::trySpawnCitizenInChunk(chunk, *citizenGenInfo, random, binaryAssetLibrary,
				textureManager, entityManager))
			{
				DebugLogWarning("Couldn't spawn citizen in chunk \"" + chunk.getPosition().toString() + "\".");
			}
		}
	}
}

void ChunkManager::populateChunk(int index, const ChunkInt2 &chunkPos, const std::optional<int> &activeLevelIndex,
	const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, EntityManager &entityManager)
{
	VoxelChunk &chunk = this->getChunk(index);
	
	// Notify the entity manager about the new chunk so entities can be spawned in it.
	entityManager.addChunk(chunkPos);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		DebugAssert(!citizenGenInfo.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(*activeLevelIndex);
		chunk.init(chunkPos, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// @todo: populate chunk entirely from default empty chunk (fast copy).
		// - probably get from MapDefinition::Interior eventually.
		constexpr VoxelChunk::VoxelMeshDefID floorVoxelMeshDefID = 2;
		const VoxelChunk::VoxelMeshDefID ceilingVoxelMeshDefID = [&levelInfoDefinition]()
		{
			for (int i = 0; i < levelInfoDefinition.getVoxelTraitsDefCount(); i++)
			{
				// @todo: from the looks of this, the engine needs to care about the concept of a "ceiling"?
				const VoxelTraitsDefinition &voxelTraitsDef = levelInfoDefinition.getVoxelTraitsDef(i);
				if (voxelTraitsDef.type == ArenaTypes::VoxelType::Ceiling)
				{
					return LevelVoxelMeshDefIdToChunkVoxelMeshDefID(i); // @todo: this is probably brittle; can't assume mesh def ID -> traits def ID mapping.
				}
			}

			// No ceiling found, use air instead.
			return VoxelChunk::AIR_VOXEL_MESH_DEF_ID;
		}();

		constexpr VoxelChunk::VoxelTextureDefID floorVoxelTextureDefID = floorVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> texture def ID mapping.
		const VoxelChunk::VoxelTextureDefID ceilingVoxelTextureDefID = ceilingVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> texture def ID mapping.

		constexpr VoxelChunk::VoxelTraitsDefID floorVoxelTraitsDefID = floorVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> traits def ID mapping.
		const VoxelChunk::VoxelTraitsDefID ceilingVoxelTraitsDefID = ceilingVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> traits def ID mapping.

		const int chunkHeight = chunk.getHeight();
		for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < VoxelChunk::WIDTH; x++)
			{
				chunk.setVoxelMeshDefID(x, 0, z, floorVoxelMeshDefID);
				chunk.setVoxelTextureDefID(x, 0, z, floorVoxelTextureDefID);
				chunk.setVoxelTraitsDefID(x, 0, z, floorVoxelTraitsDefID);

				if (chunkHeight > 2)
				{
					chunk.setVoxelMeshDefID(x, 2, z, ceilingVoxelMeshDefID);
					chunk.setVoxelTextureDefID(x, 2, z, ceilingVoxelTextureDefID);
					chunk.setVoxelTraitsDefID(x, 2, z, ceilingVoxelTraitsDefID);
				}
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkVoxelInsts(chunk);
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value() && (*activeLevelIndex == 0));
		DebugAssert(citizenGenInfo.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
		chunk.init(chunkPos, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// Chunks outside the level are wrapped but only have floor voxels.		
		for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < VoxelChunk::WIDTH; x++)
			{
				auto wrapLevelVoxel = [](int voxel, int levelDim)
				{
					if (voxel >= 0)
					{
						return voxel % levelDim;
					}
					else
					{
						return MathUtils::getWrappedIndex(levelDim, voxel);
					}
				};

				const SNInt levelWidth = levelDefinition.getWidth();
				const WEInt levelDepth = levelDefinition.getDepth();

				// Convert chunk voxel to level voxel, then wrap that between 0 and level width/depth.
				const LevelInt2 levelVoxel = VoxelUtils::chunkVoxelToNewVoxel(chunkPos, VoxelInt2(x, z));
				const LevelInt2 wrappedLevelVoxel(
					wrapLevelVoxel(levelVoxel.x, levelWidth),
					wrapLevelVoxel(levelVoxel.y, levelDepth));

				const LevelDefinition::VoxelMeshDefID levelVoxelMeshDefID = levelDefinition.getVoxelMeshID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTextureDefID levelVoxelTextureDefID = levelDefinition.getVoxelTextureID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTraitsDefID levelVoxelTraitsDefID = levelDefinition.getVoxelTraitsID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = LevelVoxelMeshDefIdToChunkVoxelMeshDefID(levelVoxelMeshDefID);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setVoxelMeshDefID(x, 0, z, voxelMeshDefID);
				chunk.setVoxelTextureDefID(x, 0, z, voxelTextureDefID);
				chunk.setVoxelTraitsDefID(x, 0, z, voxelTraitsDefID);
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkVoxelInsts(chunk);
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// The wilderness doesn't have an active level index since it's always just the one level.
		DebugAssert(!activeLevelIndex.has_value() || (*activeLevelIndex == 0));

		DebugAssert(citizenGenInfo.has_value());
		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(levelDefIndex);
		chunk.init(chunkPos, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == VoxelChunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == VoxelChunk::DEPTH);
		const LevelInt2 levelOffset = LevelInt2::Zero;
		this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
		this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);

		// Load building names for the given chunk. The wilderness might use the same level definition in
		// multiple places, so the building names have to be generated separately.
		const MapGeneration::WildChunkBuildingNameInfo *buildingNameInfo = mapDefWild.getBuildingNameInfo(chunkPos);
		if (buildingNameInfo != nullptr)
		{
			this->populateWildChunkBuildingNames(chunk, *buildingNameInfo, levelInfoDefinition);
		}

		this->populateChunkVoxelInsts(chunk);
		this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
			citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}
}

void ChunkManager::updateChunkPerimeter(VoxelChunk &chunk)
{
	auto tryUpdateChasm = [this, &chunk](const VoxelInt3 &voxel)
	{
		const CoordInt3 coord(chunk.getPosition(), voxel);
		auto getChasmFaces = [this, &coord](bool *outNorth, bool *outEast, bool *outSouth, bool *outWest)
		{
			auto getChasmFace = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelMeshDefID meshDefID)
			{
				if (chunkIndex.has_value())
				{
					const VoxelChunk &chunk = this->getChunk(*chunkIndex);
					const VoxelMeshDefinition &meshDef = chunk.getVoxelMeshDef(meshDefID);
					return meshDef.enablesNeighborGeometry;
				}
				else
				{
					return false;
				}
			};

			std::optional<int> outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex;
			VoxelChunk::VoxelMeshDefID northDefID, eastDefID, southDefID, westDefID;
			this->getAdjacentVoxelMeshDefIDs(coord, &outNorthChunkIndex, &outEastChunkIndex, &outSouthChunkIndex,
				&outWestChunkIndex, &northDefID, &eastDefID, &southDefID, &westDefID);

			*outNorth = getChasmFace(outNorthChunkIndex, northDefID);
			*outEast = getChasmFace(outEastChunkIndex, eastDefID);
			*outSouth = getChasmFace(outSouthChunkIndex, southDefID);
			*outWest = getChasmFace(outWestChunkIndex, westDefID);
		};
		
		constexpr VoxelInstance::Type voxelInstType = VoxelInstance::Type::Chasm;
		int chasmInstIndex;
		if (chunk.tryGetVoxelInstIndex(voxel.x, voxel.y, voxel.z, voxelInstType, &chasmInstIndex))
		{
			// The voxel instance already exists. See if it should be updated or removed.
			bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
			getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

			if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
			{
				// The voxel instance is still needed. Update its chasm walls.
				VoxelInstance &chasmInst = chunk.getVoxelInst(chasmInstIndex);
				VoxelInstance::ChasmState &chasmState = chasmInst.getChasmState();
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
			const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getVoxelTraitsDefID(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk.getVoxelTraitsDef(voxelTraitsDefID);
			if (voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm)
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
	for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			constexpr SNInt northX = 0;
			constexpr SNInt southX = VoxelChunk::WIDTH - 1;
			tryUpdateChasm(VoxelInt3(northX, y, z));
			tryUpdateChasm(VoxelInt3(southX, y, z));
		}
	}

	// East and west sides, minus the corners.
	for (SNInt x = 1; x < (VoxelChunk::WIDTH - 1); x++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			constexpr WEInt eastZ = 0;
			constexpr WEInt westZ = VoxelChunk::DEPTH - 1;
			tryUpdateChasm(VoxelInt3(x, y, eastZ));
			tryUpdateChasm(VoxelInt3(x, y, westZ));
		}
	}
}

void ChunkManager::update(double dt, const ChunkInt2 &centerChunkPos, const CoordDouble3 &playerCoord,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
	int chunkDistance, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager,
	EntityManager &entityManager)
{
	this->centerChunkPos = centerChunkPos;

	// Free any out-of-range chunks.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		const VoxelChunkPtr &chunkPtr = this->activeChunks[i];
		const ChunkInt2 chunkPos = chunkPtr->getPosition();
		if (!ChunkUtils::isWithinActiveRange(centerChunkPos, chunkPos, chunkDistance))
		{
			this->recycleChunk(i);

			// Notify entity manager that the chunk is being recycled.
			entityManager.removeChunk(chunkPos);
		}
	}

	// Add new chunks until the area around the center chunk is filled.
	ChunkInt2 minChunkPos, maxChunkPos;
	ChunkUtils::getSurroundingChunks(centerChunkPos, chunkDistance, &minChunkPos, &maxChunkPos);

	for (WEInt y = minChunkPos.y; y <= maxChunkPos.y; y++)
	{
		for (SNInt x = minChunkPos.x; x <= maxChunkPos.x; x++)
		{
			const ChunkInt2 curChunkPos(x, y);
			const std::optional<int> index = this->tryGetChunkIndex(curChunkPos);
			if (!index.has_value())
			{
				const int spawnIndex = this->spawnChunk();
				this->populateChunk(spawnIndex, curChunkPos, activeLevelIndex, mapDefinition, entityGenInfo,
					citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);
			}
		}
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	// Update each chunk so they can animate/destroy faded voxel instances, etc..
	const int activeChunkCount = static_cast<int>(this->activeChunks.size());
	for (int i = 0; i < activeChunkCount; i++)
	{
		VoxelChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->update(dt, playerCoord, ceilingScale, audioManager);
	}

	// Update chunk perimeters in case voxels on the edge of one chunk affect context-sensitive
	// voxels in adjacent chunks. It might be a little inefficient to do this every frame, but
	// it easily handles cases of modified voxels on a chunk edge, and it is only updating chunk
	// edges, so it should be very fast.
	for (int i = 0; i < activeChunkCount; i++)
	{
		VoxelChunkPtr &chunkPtr = this->activeChunks[i];
		this->updateChunkPerimeter(*chunkPtr);
	}
}

void ChunkManager::cleanUp()
{
	for (int i = 0; i < this->getChunkCount(); i++)
	{
		VoxelChunk &chunk = this->getChunk(i);
		chunk.clearDirtyVoxels();
	}
}
