#include <algorithm>
#include <unordered_map>

#include "VoxelChunkManager.h"
#include "../Assets/ArenaTypes.h"
#include "../Game/Game.h"
#include "../World/ChunkUtils.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

namespace
{
	VoxelChunk::VoxelShapeDefID LevelVoxelShapeDefIdToChunkVoxelShapeDefID(LevelDefinition::VoxelShapeDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelChunk::VoxelShapeDefID>(levelVoxelDefID + 1);
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

void VoxelChunkManager::getAdjacentVoxelShapeDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelChunk::VoxelShapeDefID *outNorthID, VoxelChunk::VoxelShapeDefID *outEastID, VoxelChunk::VoxelShapeDefID *outSouthID,
	VoxelChunk::VoxelShapeDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelChunk::VoxelShapeDefID>(coord, voxelIdFunc, VoxelChunk::AIR_SHAPE_DEF_ID,
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

void VoxelChunkManager::populateChunkVoxelDefs(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition)
{
	// @todo: VoxelChunk is getting two air voxel shape defs (one from VoxelChunk, one from LevelInfoDefinition).
	// - maybe clear() the defs first?
	for (int i = 0; i < levelInfoDefinition.getVoxelShapeDefCount(); i++)
	{
		VoxelShapeDefinition voxelShapeDef = levelInfoDefinition.getVoxelShapeDef(i);
		chunk.addShapeDef(std::move(voxelShapeDef));
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

	// Add floor replacement definitions and IDs.
	const LevelDefinition::VoxelShapeDefID levelFloorReplacementVoxelShapeDefID = levelDefinition.getFloorReplacementShapeDefID();
	const LevelDefinition::VoxelTextureDefID levelFloorReplacementVoxelTextureDefID = levelDefinition.getFloorReplacementTextureDefID();
	const LevelDefinition::VoxelTraitsDefID levelFloorReplacementVoxelTraitsDefID = levelDefinition.getFloorReplacementTraitsDefID();
	const LevelDefinition::ChasmDefID levelFloorReplacementChasmDefID = levelDefinition.getFloorReplacementChasmDefID();
	VoxelShapeDefinition floorReplacementShapeDef = levelInfoDefinition.getVoxelShapeDef(levelFloorReplacementVoxelShapeDefID);
	VoxelTextureDefinition floorReplacementTextureDef = levelInfoDefinition.getVoxelTextureDef(levelFloorReplacementVoxelTextureDefID);
	VoxelTraitsDefinition floorReplacementTraitsDef = levelInfoDefinition.getVoxelTraitsDef(levelFloorReplacementVoxelTraitsDefID);
	ChasmDefinition floorReplacementChasmDef = levelInfoDefinition.getChasmDef(levelFloorReplacementChasmDefID);
	const VoxelChunk::VoxelShapeDefID floorReplacementVoxelShapeDefID = chunk.addShapeDef(std::move(floorReplacementShapeDef));
	const VoxelChunk::VoxelTextureDefID floorReplacementVoxelTextureDefID = chunk.addTextureDef(std::move(floorReplacementTextureDef));
	const VoxelChunk::VoxelTraitsDefID floorReplacementVoxelTraitsDefID = chunk.addTraitsDef(std::move(floorReplacementTraitsDef));
	const VoxelChunk::ChasmDefID floorReplacementChasmDefID = chunk.addChasmDef(std::move(floorReplacementChasmDef));
	chunk.setFloorReplacementShapeDefID(floorReplacementVoxelShapeDefID);
	chunk.setFloorReplacementTextureDefID(floorReplacementVoxelTextureDefID);
	chunk.setFloorReplacementTraitsDefID(floorReplacementVoxelTraitsDefID);
	chunk.setFloorReplacementChasmDefID(floorReplacementChasmDefID);
}

void VoxelChunkManager::populateChunkVoxels(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const WorldInt2 &levelOffset)
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
				const LevelDefinition::VoxelShapeDefID levelVoxelShapeDefID = levelDefinition.getVoxelShapeID(x, y, z);
				const LevelDefinition::VoxelTextureDefID levelVoxelTextureDefID = levelDefinition.getVoxelTextureID(x, y, z);
				const LevelDefinition::VoxelTraitsDefID levelVoxelTraitsDefID = levelDefinition.getVoxelTraitsID(x, y, z);
				const VoxelChunk::VoxelShapeDefID voxelShapeDefID = LevelVoxelShapeDefIdToChunkVoxelShapeDefID(levelVoxelShapeDefID);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setShapeDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelShapeDefID);
				chunk.setTextureDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTextureDefID);
				chunk.setTraitsDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTraitsDefID);
			}
		}
	}
}

void VoxelChunkManager::populateChunkDecorators(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset)
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
		for (const WorldInt3 &position : placementDef.positions)
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
		for (const WorldInt3 &position : placementDef.positions)
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
		for (const WorldInt3 &position : placementDef.positions)
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
		for (const WorldInt3 &position : placementDef.positions)
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
		for (const WorldInt3 &position : placementDef.positions)
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
		for (const WorldInt3 &position : placementDef.positions)
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
					buildingNameIDs.emplace(buildingNameID, chunkBuildingNameID);
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
				const VoxelChunk::VoxelShapeDefID voxelShapeDefID = chunk.getShapeDefID(x, y, z);
				const VoxelShapeDefinition &voxelShapeDef = chunk.getShapeDef(voxelShapeDefID);
				if (!voxelShapeDef.isContextSensitive)
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
				VoxelChunk::VoxelShapeDefID northVoxelShapeDefID, eastVoxelShapeDefID, southVoxelShapeDefID, westVoxelShapeDefID;
				this->getAdjacentVoxelShapeDefIDs(coord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
					&northVoxelShapeDefID, &eastVoxelShapeDefID, &southVoxelShapeDefID, &westVoxelShapeDefID);

				auto isFaceActive = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelShapeDefID shapeDefID)
				{
					if (!chunkIndex.has_value())
					{
						return false;
					}

					const VoxelChunk &voxelChunk = this->getChunkAtIndex(*chunkIndex);
					const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);
					return shapeDef.enablesNeighborGeometry;
				};

				bool hasNorthFace = isFaceActive(northChunkIndex, northVoxelShapeDefID);
				bool hasEastFace = isFaceActive(eastChunkIndex, eastVoxelShapeDefID);
				bool hasSouthFace = isFaceActive(southChunkIndex, southVoxelShapeDefID);
				bool hasWestFace = isFaceActive(westChunkIndex, westVoxelShapeDefID);

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
	DebugAssert(chunk.getDoorVisibilityInsts().getCount() == 0);

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

void VoxelChunkManager::populateChunk(int index, const ChunkInt2 &chunkPos, const LevelDefinition &levelDef,
	const LevelInfoDefinition &levelInfoDef, const MapSubDefinition &mapSubDef)
{
	VoxelChunk &chunk = this->getChunkAtIndex(index);
	const SNInt levelWidth = levelDef.getWidth();
	const int levelHeight = levelDef.getHeight();
	const WEInt levelDepth = levelDef.getDepth();

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapSubDef.type;
	if (mapType == MapType::Interior)
	{
		chunk.init(chunkPos, levelHeight);
		this->populateChunkVoxelDefs(chunk, levelDef, levelInfoDef);

		// @todo: populate chunk entirely from default empty chunk (fast copy).
		// - probably get from MapDefinitionInterior eventually.
		constexpr VoxelChunk::VoxelShapeDefID floorVoxelShapeDefID = 2;
		const VoxelChunk::VoxelShapeDefID ceilingVoxelShapeDefID = [&levelInfoDef]()
		{
			for (int i = 0; i < levelInfoDef.getVoxelTraitsDefCount(); i++)
			{
				// @todo: from the looks of this, the engine needs to care about the concept of a "ceiling"?
				const VoxelTraitsDefinition &voxelTraitsDef = levelInfoDef.getVoxelTraitsDef(i);
				if (voxelTraitsDef.type == ArenaTypes::VoxelType::Ceiling)
				{
					return LevelVoxelShapeDefIdToChunkVoxelShapeDefID(i); // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.
				}
			}

			// No ceiling found, use air instead.
			return VoxelChunk::AIR_SHAPE_DEF_ID;
		}();

		constexpr VoxelChunk::VoxelTextureDefID floorVoxelTextureDefID = floorVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> texture def ID mapping.
		const VoxelChunk::VoxelTextureDefID ceilingVoxelTextureDefID = ceilingVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> texture def ID mapping.

		constexpr VoxelChunk::VoxelTraitsDefID floorVoxelTraitsDefID = floorVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.
		const VoxelChunk::VoxelTraitsDefID ceilingVoxelTraitsDefID = ceilingVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.

		const int chunkHeight = chunk.getHeight();
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				chunk.setShapeDefID(x, 0, z, floorVoxelShapeDefID);
				chunk.setTextureDefID(x, 0, z, floorVoxelTextureDefID);
				chunk.setTraitsDefID(x, 0, z, floorVoxelTraitsDefID);

				if (chunkHeight > 2)
				{
					chunk.setShapeDefID(x, 2, z, ceilingVoxelShapeDefID);
					chunk.setTextureDefID(x, 2, z, ceilingVoxelTextureDefID);
					chunk.setTraitsDefID(x, 2, z, ceilingVoxelTraitsDefID);
				}
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelWidth, levelDepth))
		{
			// Populate chunk from the part of the level it overlaps.
			const WorldInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDef, levelOffset);
			this->populateChunkDecorators(chunk, levelDef, levelInfoDef, levelOffset);
			this->populateChunkChasmInsts(chunk);
			this->populateChunkDoorVisibilityInsts(chunk);
		}
	}
	else if (mapType == MapType::City)
	{
		chunk.init(chunkPos, levelHeight);
		this->populateChunkVoxelDefs(chunk, levelDef, levelInfoDef);

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

				// Convert chunk voxel to level voxel, then wrap that between 0 and level width/depth.
				const WorldInt2 levelVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt2(x, z));
				const WorldInt2 wrappedLevelVoxel(
					wrapLevelVoxel(levelVoxel.x, levelWidth),
					wrapLevelVoxel(levelVoxel.y, levelDepth));

				const LevelDefinition::VoxelShapeDefID levelVoxelShapeDefID = levelDef.getVoxelShapeID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTextureDefID levelVoxelTextureDefID = levelDef.getVoxelTextureID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelDefinition::VoxelTraitsDefID levelVoxelTraitsDefID = levelDef.getVoxelTraitsID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const VoxelChunk::VoxelShapeDefID voxelShapeDefID = LevelVoxelShapeDefIdToChunkVoxelShapeDefID(levelVoxelShapeDefID);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setShapeDefID(x, 0, z, voxelShapeDefID);
				chunk.setTextureDefID(x, 0, z, voxelTextureDefID);
				chunk.setTraitsDefID(x, 0, z, voxelTraitsDefID);
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelWidth, levelDepth))
		{
			// Populate chunk from the part of the level it overlaps.
			const WorldInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDef, levelOffset);
			this->populateChunkDecorators(chunk, levelDef, levelInfoDef, levelOffset);
			this->populateChunkChasmInsts(chunk);
			this->populateChunkDoorVisibilityInsts(chunk);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		chunk.init(chunkPos, levelHeight);
		this->populateChunkVoxelDefs(chunk, levelDef, levelInfoDef);

		// Copy level definition directly into chunk.
		DebugAssert(levelWidth == Chunk::WIDTH);
		DebugAssert(levelDepth == Chunk::DEPTH);
		const WorldInt2 levelOffset = WorldInt2::Zero;
		this->populateChunkVoxels(chunk, levelDef, levelOffset);
		this->populateChunkDecorators(chunk, levelDef, levelInfoDef, levelOffset);

		// Load building names for the given chunk. The wilderness might use the same level definition in
		// multiple places, so the building names have to be generated separately.
		const MapDefinitionWild &mapDefWild = mapSubDef.wild;
		const MapGeneration::WildChunkBuildingNameInfo *buildingNameInfo = mapDefWild.getBuildingNameInfo(chunkPos);
		if (buildingNameInfo != nullptr)
		{
			this->populateWildChunkBuildingNames(chunk, *buildingNameInfo, levelInfoDef);
		}

		this->populateChunkChasmInsts(chunk);
		this->populateChunkDoorVisibilityInsts(chunk);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}
}

void VoxelChunkManager::updateChasmWallInst(VoxelChunk &chunk, SNInt x, int y, WEInt z)
{
	const VoxelInt3 voxel(x, y, z);
	const CoordInt3 coord(chunk.getPosition(), voxel);
	auto getChasmFaces = [this, &coord](bool *outNorth, bool *outEast, bool *outSouth, bool *outWest)
	{
		auto getChasmFace = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelShapeDefID shapeDefID)
		{
			if (chunkIndex.has_value())
			{
				const VoxelChunk &chunk = this->getChunkAtIndex(*chunkIndex);
				const VoxelShapeDefinition &shapeDef = chunk.getShapeDef(shapeDefID);
				return shapeDef.enablesNeighborGeometry;
			}
			else
			{
				return false;
			}
		};

		std::optional<int> outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex;
		VoxelChunk::VoxelShapeDefID northDefID, eastDefID, southDefID, westDefID;
		this->getAdjacentVoxelShapeDefIDs(coord, &outNorthChunkIndex, &outEastChunkIndex, &outSouthChunkIndex,
			&outWestChunkIndex, &northDefID, &eastDefID, &southDefID, &westDefID);

		*outNorth = getChasmFace(outNorthChunkIndex, northDefID);
		*outEast = getChasmFace(outEastChunkIndex, eastDefID);
		*outSouth = getChasmFace(outSouthChunkIndex, southDefID);
		*outWest = getChasmFace(outWestChunkIndex, westDefID);
	};

	int chasmInstIndex;
	if (chunk.tryGetChasmWallInstIndex(x, y, z, &chasmInstIndex))
	{
		// The chasm wall instance already exists. See if it should be updated or removed.
		bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
		getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

		if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
		{
			// The instance is still needed. Update its chasm walls.
			BufferView<VoxelChasmWallInstance> chasmWallInsts = chunk.getChasmWallInsts();
			VoxelChasmWallInstance &chasmWallInst = chasmWallInsts[chasmInstIndex];

			const bool shouldDirtyChasmWallInst = (chasmWallInst.north != hasNorthFace) || (chasmWallInst.east != hasEastFace) ||
				(chasmWallInst.south != hasSouthFace) || (chasmWallInst.west != hasWestFace);

			chasmWallInst.north = hasNorthFace;
			chasmWallInst.east = hasEastFace;
			chasmWallInst.south = hasSouthFace;
			chasmWallInst.west = hasWestFace;

			if (shouldDirtyChasmWallInst)
			{
				chunk.addDirtyChasmWallInstPosition(voxel);
			}
		}
		else
		{
			// The chasm wall instance no longer has any interesting data.
			chunk.removeChasmWallInst(voxel);
			chunk.addDirtyChasmWallInstPosition(voxel);
		}
	}
	else
	{
		// No instance yet. If it's a chasm, add a new voxel instance.
		const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(x, y, z);
		const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
		if (voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm)
		{
			bool hasNorthFace, hasEastFace, hasSouthFace, hasWestFace;
			getChasmFaces(&hasNorthFace, &hasEastFace, &hasSouthFace, &hasWestFace);

			if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
			{
				VoxelChasmWallInstance chasmWallInst;
				chasmWallInst.init(x, y, z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
				chunk.addChasmWallInst(std::move(chasmWallInst));
				chunk.addDirtyChasmWallInstPosition(voxel);
			}
		}
	}
}

void VoxelChunkManager::updateChunkDoorVisibilityInsts(VoxelChunk &chunk, const CoordDouble3 &playerCoord)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();
	const CoordInt3 playerCoordInt(playerCoord.chunk, VoxelUtils::pointToVoxel(playerCoord.point));

	for (VoxelDoorVisibilityInstance &visInst : chunk.getDoorVisibilityInsts())
	{
		const CoordInt3 doorCoord(chunkPos, VoxelInt3(visInst.x, visInst.y, visInst.z));

		const bool isCameraNorthInclusive = (playerCoordInt.chunk.x < doorCoord.chunk.x) ||
			((playerCoordInt.chunk.x == doorCoord.chunk.x) && (playerCoordInt.voxel.x <= doorCoord.voxel.x));
		const bool isCameraEastInclusive = (playerCoordInt.chunk.y < doorCoord.chunk.y) ||
			((playerCoordInt.chunk.y == doorCoord.chunk.y) && (playerCoordInt.voxel.z <= doorCoord.voxel.z));

		std::optional<int> northChunkIndex, eastChunkIndex, southChunkIndex, westChunkIndex;
		VoxelChunk::VoxelShapeDefID northVoxelShapeDefID, eastVoxelShapeDefID, southVoxelShapeDefID, westVoxelShapeDefID;
		this->getAdjacentVoxelShapeDefIDs(doorCoord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
			&northVoxelShapeDefID, &eastVoxelShapeDefID, &southVoxelShapeDefID, &westVoxelShapeDefID);

		auto isVoxelValidForDoorFace = [this](const std::optional<int> &chunkIndex, VoxelChunk::VoxelShapeDefID shapeDefID)
		{
			if (!chunkIndex.has_value())
			{
				return true;
			}

			const VoxelChunk &voxelChunk = this->getChunkAtIndex(*chunkIndex);
			const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);
			return (shapeDef.type == VoxelShapeType::None) || shapeDef.allowsAdjacentDoorFaces;
		};

		const bool isNorthValid = isVoxelValidForDoorFace(northChunkIndex, northVoxelShapeDefID);
		const bool isEastValid = isVoxelValidForDoorFace(eastChunkIndex, eastVoxelShapeDefID);
		const bool isSouthValid = isVoxelValidForDoorFace(southChunkIndex, southVoxelShapeDefID);
		const bool isWestValid = isVoxelValidForDoorFace(westChunkIndex, westVoxelShapeDefID);

		visInst.update(isCameraNorthInclusive, isCameraEastInclusive, isNorthValid, isEastValid, isSouthValid, isWestValid);
		chunk.addDirtyDoorVisInstPosition(doorCoord.voxel);
	}
}

void VoxelChunkManager::update(double dt, BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const CoordDouble3 &playerCoord, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
	const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs, BufferView<const int> levelInfoDefIndices,
	BufferView<const LevelInfoDefinition> levelInfoDefs, double ceilingScale, AudioManager &audioManager)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	const MapType mapType = mapSubDef.type;
	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const int spawnIndex = this->spawnChunk();

		// Default to the active level def unless it's the wilderness which relies on this chunk coordinate.
		const LevelDefinition *levelDefPtr = activeLevelDef;
		const LevelInfoDefinition *levelInfoDefPtr = activeLevelInfoDef;
		if (mapType == MapType::Wilderness)
		{
			const MapDefinitionWild &mapDefWild = mapSubDef.wild;
			const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
			levelDefPtr = &levelDefs[levelDefIndex];

			const int levelInfoDefIndex = levelInfoDefIndices[levelDefIndex];
			levelInfoDefPtr = &levelInfoDefs[levelInfoDefIndex];
		}

		this->populateChunk(spawnIndex, chunkPos, *levelDefPtr, *levelInfoDefPtr, mapSubDef);
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

	// Check if new chasms caused surrounding chasms to become dirty.
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		BufferView<const VoxelInt3> oldDirtyChasmWallPositions = chunkPtr->getDirtyChasmWallInstPositions();
		if (oldDirtyChasmWallPositions.getCount() == 0)
		{
			continue;
		}

		// Cache the existing dirty chasm walls since they get invalidated below.
		Buffer<VoxelInt3> cachedDirtyChasmWallPositions(oldDirtyChasmWallPositions.getCount());
		std::copy(oldDirtyChasmWallPositions.begin(), oldDirtyChasmWallPositions.end(), cachedDirtyChasmWallPositions.begin());

		for (const VoxelInt3 &dirtyChasmWallPos : cachedDirtyChasmWallPositions)
		{
			const CoordInt3 coord(chunkPtr->getPosition(), dirtyChasmWallPos);
			const CoordInt3 adjacentCoords[] =
			{
				VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::North),
				VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::East),
				VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::South),
				VoxelUtils::getAdjacentCoordXZ(coord, VoxelUtils::West)
			};

			for (const CoordInt3 &adjacentCoord : adjacentCoords)
			{
				const std::optional<int> adjacentChunkIndex = this->tryGetChunkIndex(adjacentCoord.chunk);
				if (adjacentChunkIndex.has_value())
				{
					VoxelChunk &adjacentChunk = this->getChunkAtIndex(*adjacentChunkIndex);
					const VoxelInt3 &adjacentVoxel = adjacentCoord.voxel;
					int dummyChasmWallInstIndex;
					if (adjacentChunk.tryGetChasmWallInstIndex(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z, &dummyChasmWallInstIndex))
					{
						adjacentChunk.addDirtyChasmWallInstPosition(adjacentVoxel);
					}
				}
			}
		}
	}

	// Update chasm wall instances that may be dirty from fading voxels in this chunk or adjacent chunks,
	// or an adjacent chunk that was wholly added or removed this frame.
	for (int i = 0; i < activeChunkCount; i++)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		VoxelChunk &chunk = *chunkPtr;

		for (const VoxelInt3 &chasmWallPos : chunk.getDirtyChasmWallInstPositions())
		{
			this->updateChasmWallInst(chunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		// North and south sides.
		const int chunkHeight = chunk.getHeight();
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (int y = 0; y < chunkHeight; y++)
			{
				constexpr SNInt northX = 0;
				constexpr SNInt southX = Chunk::WIDTH - 1;
				this->updateChasmWallInst(chunk, northX, y, z);
				this->updateChasmWallInst(chunk, southX, y, z);
			}
		}

		// East and west sides, minus the corners.
		for (SNInt x = 1; x < (Chunk::WIDTH - 1); x++)
		{
			for (int y = 0; y < chunkHeight; y++)
			{
				constexpr WEInt eastZ = 0;
				constexpr WEInt westZ = Chunk::DEPTH - 1;
				this->updateChasmWallInst(chunk, x, y, eastZ);
				this->updateChasmWallInst(chunk, x, y, westZ);
			}
		}
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
		chunkPtr->cleanUp();
	}
}
