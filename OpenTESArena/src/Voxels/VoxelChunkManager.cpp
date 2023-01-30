#include <algorithm>
#include <unordered_map>

#include "VoxelChunkManager.h"
#include "../Assets/ArenaTypes.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../World/ChunkUtils.h"
#include "../World/MapDefinition.h"
#include "../World/MapInstance.h"
#include "../World/MapType.h"

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

void VoxelChunkManager::getAdjacentVoxelMeshDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelMeshDefID *outNorthID, VoxelChunk::VoxelMeshDefID *outEastID, VoxelChunk::VoxelMeshDefID *outSouthID,
	VoxelChunk::VoxelMeshDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getMeshDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelChunk::VoxelMeshDefID>(coord, voxelIdFunc, VoxelChunk::AIR_MESH_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::getAdjacentVoxelTextureDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelTextureDefID *outNorthID, VoxelChunk::VoxelTextureDefID *outEastID, VoxelChunk::VoxelTextureDefID *outSouthID,
	VoxelChunk::VoxelTextureDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getTextureDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelChunk::VoxelTextureDefID>(coord, voxelIdFunc, VoxelChunk::AIR_TEXTURE_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::getAdjacentVoxelTraitsDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelTraitsDefID *outNorthID, VoxelChunk::VoxelTraitsDefID *outEastID, VoxelChunk::VoxelTraitsDefID *outSouthID,
	VoxelChunk::VoxelTraitsDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelChunk::VoxelTraitsDefID>(coord, voxelIdFunc, VoxelChunk::AIR_TRAITS_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::populateChunkVoxelDefs(VoxelChunk &chunk, const LevelInfoDefinition &levelInfoDefinition)
{
	for (int i = 0; i < levelInfoDefinition.getVoxelMeshDefCount(); i++)
	{
		VoxelMeshDefinition voxelMeshDef = levelInfoDefinition.getVoxelMeshDef(i);
		chunk.addMeshDef(std::move(voxelMeshDef));
	}

	for (int i = 0; i < levelInfoDefinition.getVoxelTextureDefCount(); i++)
	{
		VoxelTextureDefinition voxelTextureDef = levelInfoDefinition.getVoxelTextureDef(i);
		chunk.addTextureDef(std::move(voxelTextureDef));
	}

	for (int i = 0; i < levelInfoDefinition.getVoxelTraitsDefCount(); i++)
	{
		VoxelTraitsDefinition voxelTraitsDef = levelInfoDefinition.getVoxelTraitsDef(i);
		chunk.addTraitsDef(std::move(voxelTraitsDef));
	}
}

void VoxelChunkManager::populateChunkVoxels(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
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
				chunk.setMeshDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelMeshDefID);
				chunk.setTextureDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTextureDefID);
				chunk.setTraitsDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTraitsDefID);
			}
		}
	}
}

void VoxelChunkManager::populateChunkDecorators(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
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

void VoxelChunkManager::populateWildChunkBuildingNames(VoxelChunk &chunk,
	const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo, const LevelInfoDefinition &levelInfoDefinition)
{
	// Cache of level building names that have been added to the chunk.
	std::unordered_map<LevelDefinition::BuildingNameID, VoxelChunk::BuildingNameID> buildingNameIDs;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
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

void VoxelChunkManager::populateChunkChasmInsts(VoxelChunk &chunk)
{
	// @todo: only iterate over chunk writing ranges

	const ChunkInt2 &chunkPos = chunk.getPosition();
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = chunk.getMeshDefID(x, y, z);
				const VoxelMeshDefinition &voxelMeshDef = chunk.getMeshDef(voxelMeshDefID);
				if (!voxelMeshDef.isContextSensitive)
				{
					continue;
				}

				int chasmInstIndex;
				if (chunk.tryGetChasmWallInstIndex(x, y, z, &chasmInstIndex))
				{
					DebugLogError("Expected no existing chasm wall instance at (" + std::to_string(x) + ", " +
						std::to_string(y) + ", " + std::to_string(z) + ") in chunk (" + chunkPos.toString() + ").");
					continue;
				}

				const CoordInt3 coord(chunkPos, VoxelInt3(x, y, z));
				std::optional<int> northChunkIndex, eastChunkIndex, southChunkIndex, westChunkIndex;
				VoxelChunk::VoxelMeshDefID northVoxelMeshDefID, eastVoxelMeshDefID, southVoxelMeshDefID, westVoxelMeshDefID;
				this->getAdjacentVoxelMeshDefIDs(coord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
					&northVoxelMeshDefID, &eastVoxelMeshDefID, &southVoxelMeshDefID, &westVoxelMeshDefID);

				auto isFaceActive = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelMeshDefID meshDefID)
				{
					if (!chunkIndex.has_value())
					{
						return false;
					}

					const VoxelChunk &voxelChunk = this->getChunkAtIndex(*chunkIndex);
					const VoxelMeshDefinition &meshDef = voxelChunk.getMeshDef(meshDefID);
					return meshDef.enablesNeighborGeometry;
				};

				bool hasNorthFace = isFaceActive(northChunkIndex, northVoxelMeshDefID);
				bool hasEastFace = isFaceActive(eastChunkIndex, eastVoxelMeshDefID);
				bool hasSouthFace = isFaceActive(southChunkIndex, southVoxelMeshDefID);
				bool hasWestFace = isFaceActive(westChunkIndex, westVoxelMeshDefID);

				if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
				{
					VoxelChasmWallInstance chasmWallInst;
					chasmWallInst.init(x, y, z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
					chunk.addChasmWallInst(std::move(chasmWallInst));
				}
			}
		}
	}
}

void VoxelChunkManager::populateChunkDoorVisibilityInsts(VoxelChunk &chunk)
{
	DebugAssert(chunk.getDoorVisibilityInstCount() == 0);

	const ChunkInt2 &chunkPos = chunk.getPosition();
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				VoxelChunk::DoorDefID doorDefID;
				if (chunk.tryGetDoorDefID(x, y, z, &doorDefID))
				{
					VoxelDoorVisibilityInstance doorVisInst;
					doorVisInst.init(x, y, z);
					chunk.addDoorVisibilityInst(std::move(doorVisInst));
				}
			}
		}
	}
}

void VoxelChunkManager::populateChunk(int index, const ChunkInt2 &chunkPos, const std::optional<int> &activeLevelIndex,
	const MapDefinition &mapDefinition, const MapInstance &mapInstance)
{
	VoxelChunk &chunk = this->getChunkAtIndex(index);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapInstance.getLevelInfoForLevel(*activeLevelIndex);
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
			return VoxelChunk::AIR_MESH_DEF_ID;
		}();

		constexpr VoxelChunk::VoxelTextureDefID floorVoxelTextureDefID = floorVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> texture def ID mapping.
		const VoxelChunk::VoxelTextureDefID ceilingVoxelTextureDefID = ceilingVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> texture def ID mapping.

		constexpr VoxelChunk::VoxelTraitsDefID floorVoxelTraitsDefID = floorVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> traits def ID mapping.
		const VoxelChunk::VoxelTraitsDefID ceilingVoxelTraitsDefID = ceilingVoxelMeshDefID; // @todo: this is probably brittle; can't assume mesh def ID -> traits def ID mapping.

		const int chunkHeight = chunk.getHeight();
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				chunk.setMeshDefID(x, 0, z, floorVoxelMeshDefID);
				chunk.setTextureDefID(x, 0, z, floorVoxelTextureDefID);
				chunk.setTraitsDefID(x, 0, z, floorVoxelTraitsDefID);

				if (chunkHeight > 2)
				{
					chunk.setMeshDefID(x, 2, z, ceilingVoxelMeshDefID);
					chunk.setTextureDefID(x, 2, z, ceilingVoxelTextureDefID);
					chunk.setTraitsDefID(x, 2, z, ceilingVoxelTraitsDefID);
				}
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkChasmInsts(chunk);
			this->populateChunkDoorVisibilityInsts(chunk);

			/*DebugAssert(!citizenGenInfo.has_value());
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);*/
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value() && (*activeLevelIndex == 0));
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapInstance.getLevelInfoForLevel(0);
		chunk.init(chunkPos, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// Chunks outside the level are wrapped but only have floor voxels.		
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
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
				const LevelInt2 levelVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt2(x, z));
				const LevelInt2 wrappedLevelVoxel(
					wrapLevelVoxel(levelVoxel.x, levelWidth),
					wrapLevelVoxel(levelVoxel.y, levelDepth));

				const LevelDefinition::VoxelMeshDefID levelVoxelMeshDefID = levelDefinition.getVoxelMeshID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTextureDefID levelVoxelTextureDefID = levelDefinition.getVoxelTextureID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTraitsDefID levelVoxelTraitsDefID = levelDefinition.getVoxelTraitsID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = LevelVoxelMeshDefIdToChunkVoxelMeshDefID(levelVoxelMeshDefID);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setMeshDefID(x, 0, z, voxelMeshDefID);
				chunk.setTextureDefID(x, 0, z, voxelTextureDefID);
				chunk.setTraitsDefID(x, 0, z, voxelTraitsDefID);
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDefinition, levelOffset);
			this->populateChunkDecorators(chunk, levelDefinition, levelInfoDefinition, levelOffset);
			this->populateChunkChasmInsts(chunk);
			this->populateChunkDoorVisibilityInsts(chunk);

			/*DebugAssert(citizenGenInfo.has_value());
			this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);*/
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// The wilderness doesn't have an active level index since it's always just the one level.
		DebugAssert(!activeLevelIndex.has_value() || (*activeLevelIndex == 0));

		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapInstance.getLevelInfoForLevel(levelDefIndex);
		chunk.init(chunkPos, levelDefinition.getHeight());
		this->populateChunkVoxelDefs(chunk, levelInfoDefinition);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
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

		this->populateChunkChasmInsts(chunk);
		this->populateChunkDoorVisibilityInsts(chunk);

		/*DebugAssert(citizenGenInfo.has_value());
		this->populateChunkEntities(chunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
			citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager, entityManager);*/
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}
}

void VoxelChunkManager::updateChunkPerimeterChasmInsts(VoxelChunk &chunk)
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
					const VoxelChunk &chunk = this->getChunkAtIndex(*chunkIndex);
					const VoxelMeshDefinition &meshDef = chunk.getMeshDef(meshDefID);
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

		int chasmInstIndex;
		if (chunk.tryGetChasmWallInstIndex(voxel.x, voxel.y, voxel.z, &chasmInstIndex))
		{
			// The chasm wall instance already exists. See if it should be updated or removed.
			bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
			getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

			if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
			{
				// The instance is still needed. Update its chasm walls.
				VoxelChasmWallInstance &chasmWallInst = chunk.getChasmWallInst(chasmInstIndex);
				chasmWallInst.north = hasNorthFace;
				chasmWallInst.east = hasEastFace;
				chasmWallInst.south = hasSouthFace;
				chasmWallInst.west = hasWestFace;
			}
			else
			{
				// The chasm wall instance no longer has any interesting data.
				chunk.removeChasmWallInst(voxel);
			}
		}
		else
		{
			// No instance yet. If it's a chasm, add a new voxel instance.
			const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
			if (voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm)
			{
				bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
				getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

				if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
				{
					VoxelChasmWallInstance chasmWallInst;
					chasmWallInst.init(voxel.x, voxel.y, voxel.z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
					chunk.addChasmWallInst(std::move(chasmWallInst));
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

void VoxelChunkManager::updateChunkDoorVisibilityInsts(VoxelChunk &chunk, const CoordDouble3 &playerCoord)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();
	const CoordInt3 playerCoordInt(playerCoord.chunk, VoxelUtils::pointToVoxel(playerCoord.point));

	for (int i = 0; i < chunk.getDoorVisibilityInstCount(); i++)
	{
		VoxelDoorVisibilityInstance &visInst = chunk.getDoorVisibilityInst(i);
		const CoordInt3 doorCoord(chunkPos, VoxelInt3(visInst.x, visInst.y, visInst.z));

		const bool isCameraNorthInclusive = (playerCoordInt.chunk.x < doorCoord.chunk.x) ||
			((playerCoordInt.chunk.x == doorCoord.chunk.x) && (playerCoordInt.voxel.x <= doorCoord.voxel.x));
		const bool isCameraEastInclusive = (playerCoordInt.chunk.y < doorCoord.chunk.y) ||
			((playerCoordInt.chunk.y == doorCoord.chunk.y) && (playerCoordInt.voxel.z <= doorCoord.voxel.z));

		std::optional<int> northChunkIndex, eastChunkIndex, southChunkIndex, westChunkIndex;
		VoxelChunk::VoxelMeshDefID northVoxelMeshDefID, eastVoxelMeshDefID, southVoxelMeshDefID, westVoxelMeshDefID;
		this->getAdjacentVoxelMeshDefIDs(doorCoord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
			&northVoxelMeshDefID, &eastVoxelMeshDefID, &southVoxelMeshDefID, &westVoxelMeshDefID);

		auto isVoxelAir = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelMeshDefID meshDefID)
		{
			if (!chunkIndex.has_value())
			{
				return true;
			}

			const VoxelChunk &voxelChunk = this->getChunkAtIndex(*chunkIndex);
			const VoxelMeshDefinition &meshDef = voxelChunk.getMeshDef(meshDefID);
			return meshDef.isEmpty();
		};

		const bool isNorthAir = isVoxelAir(northChunkIndex, northVoxelMeshDefID);
		const bool isEastAir = isVoxelAir(eastChunkIndex, eastVoxelMeshDefID);
		const bool isSouthAir = isVoxelAir(southChunkIndex, southVoxelMeshDefID);
		const bool isWestAir = isVoxelAir(westChunkIndex, westVoxelMeshDefID);
		visInst.update(isCameraNorthInclusive, isCameraEastInclusive, isNorthAir, isEastAir, isSouthAir, isWestAir);
	}
}

void VoxelChunkManager::update(double dt, const BufferView<const ChunkInt2> &newChunkPositions,
	const BufferView<const ChunkInt2> &freedChunkPositions, const CoordDouble3 &playerCoord,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition, const MapInstance &mapInstance,
	double ceilingScale, AudioManager &audioManager)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		const int spawnIndex = this->spawnChunk();
		this->populateChunk(spawnIndex, chunkPos, activeLevelIndex, mapDefinition, mapInstance);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	// Update each chunk so they can animate/destroy faded voxel instances, etc..
	const int activeChunkCount = static_cast<int>(this->activeChunks.size());
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->update(dt, playerCoord, ceilingScale, audioManager);
	}

	// Update chunk perimeters in case voxels on the edge of one chunk affect context-sensitive
	// voxels in adjacent chunks. It might be a little inefficient to do this every frame, but
	// it easily handles cases of modified voxels on a chunk edge, and it is only updating chunk
	// edges, so it should be very fast.
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->updateChunkPerimeterChasmInsts(*chunkPtr);
	}

	// Update which door faces are able to be rendered.
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->updateChunkDoorVisibilityInsts(*chunkPtr, playerCoord);
	}
}

void VoxelChunkManager::cleanUp()
{
	const int activeChunkCount = static_cast<int>(this->activeChunks.size());
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->clearDirtyVoxels();
	}
}
