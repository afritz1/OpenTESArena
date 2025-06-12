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
	VoxelShapeDefID LevelVoxelShapeDefIdToChunkVoxelShapeDefID(LevelVoxelShapeDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelShapeDefID>(levelVoxelDefID + 1);
	}

	VoxelTextureDefID LevelVoxelTextureDefIdToChunkVoxelTextureDefID(LevelVoxelTextureDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelTextureDefID>(levelVoxelDefID + 1);
	}

	VoxelTextureDefID LevelVoxelShadingDefIdToChunkVoxelShadingDefID(LevelVoxelShadingDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelShadingDefID>(levelVoxelDefID + 1);
	}

	VoxelTraitsDefID LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(LevelVoxelTraitsDefID levelVoxelDefID)
	{
		// Chunks have an air definition at ID 0.
		return static_cast<VoxelTraitsDefID>(levelVoxelDefID + 1);
	}
}

int VoxelChunkManager::getChasmDefCount() const
{
	return static_cast<int>(this->chasmDefs.size());
}

const VoxelChasmDefinition &VoxelChunkManager::getChasmDef(VoxelChasmDefID id) const
{
	DebugAssertIndex(this->chasmDefs, id);
	return this->chasmDefs[id];
}

VoxelChasmDefID VoxelChunkManager::findChasmDef(const VoxelChasmDefinition &def)
{
	for (int i = 0; i < static_cast<int>(this->chasmDefs.size()); i++)
	{
		const VoxelChasmDefinition &currentDef = this->chasmDefs[i];
		if (currentDef == def)
		{
			return i;
		}
	}

	return -1;
}

VoxelChasmDefID VoxelChunkManager::addChasmDef(VoxelChasmDefinition &&def)
{
	const VoxelChasmDefID id = static_cast<int>(this->chasmDefs.size());
	this->chasmDefs.emplace_back(std::move(def));
	return id;
}

void VoxelChunkManager::getAdjacentVoxelShapeDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelShapeDefID *outNorthID, VoxelShapeDefID *outEastID, VoxelShapeDefID *outSouthID,
	VoxelShapeDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelShapeDefID>(coord, voxelIdFunc, VoxelChunk::AIR_SHAPE_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::getAdjacentVoxelTextureDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelTextureDefID *outNorthID, VoxelTextureDefID *outEastID, VoxelTextureDefID *outSouthID,
	VoxelTextureDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getTextureDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelTextureDefID>(coord, voxelIdFunc, VoxelChunk::AIR_TEXTURE_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::getAdjacentVoxelShadingDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelShadingDefID *outNorthID, VoxelShadingDefID *outEastID, VoxelShadingDefID *outSouthID, VoxelShadingDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getShadingDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelShadingDefID>(coord, voxelIdFunc, VoxelChunk::AIR_SHADING_DEF_ID,
		outNorthChunkIndex, outEastChunkIndex, outSouthChunkIndex, outWestChunkIndex, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunkManager::getAdjacentVoxelTraitsDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
	std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
	VoxelTraitsDefID *outNorthID, VoxelTraitsDefID *outEastID, VoxelTraitsDefID *outSouthID,
	VoxelTraitsDefID *outWestID)
{
	auto voxelIdFunc = [](const VoxelChunk &chunk, const VoxelInt3 &voxel)
	{
		return chunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
	};

	this->getAdjacentVoxelIDsInternal<VoxelTraitsDefID>(coord, voxelIdFunc, VoxelChunk::AIR_TRAITS_DEF_ID,
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

	for (int i = 0; i < levelInfoDefinition.getVoxelShadingDefCount(); i++)
	{
		VoxelShadingDefinition voxelShadingDef = levelInfoDefinition.getVoxelShadingDef(i);
		chunk.addShadingDef(std::move(voxelShadingDef));
	}

	for (int i = 0; i < levelInfoDefinition.getVoxelTraitsDefCount(); i++)
	{
		VoxelTraitsDefinition voxelTraitsDef = levelInfoDefinition.getVoxelTraitsDef(i);
		chunk.addTraitsDef(std::move(voxelTraitsDef));
	}

	// Add floor replacement definitions and IDs.
	const LevelVoxelShapeDefID levelFloorReplacementVoxelShapeDefID = levelDefinition.getFloorReplacementShapeDefID();
	const LevelVoxelTextureDefID levelFloorReplacementVoxelTextureDefID = levelDefinition.getFloorReplacementTextureDefID();
	const LevelVoxelShadingDefID levelFloorReplacementVoxelShadingDefID = levelDefinition.getFloorReplacementShadingDefID();
	const LevelVoxelTraitsDefID levelFloorReplacementVoxelTraitsDefID = levelDefinition.getFloorReplacementTraitsDefID();
	VoxelShapeDefinition floorReplacementShapeDef = levelInfoDefinition.getVoxelShapeDef(levelFloorReplacementVoxelShapeDefID);
	VoxelTextureDefinition floorReplacementTextureDef = levelInfoDefinition.getVoxelTextureDef(levelFloorReplacementVoxelTextureDefID);
	VoxelShadingDefinition floorReplacementShadingDef = levelInfoDefinition.getVoxelShadingDef(levelFloorReplacementVoxelShadingDefID);
	VoxelTraitsDefinition floorReplacementTraitsDef = levelInfoDefinition.getVoxelTraitsDef(levelFloorReplacementVoxelTraitsDefID);
	const VoxelShapeDefID floorReplacementVoxelShapeDefID = chunk.addShapeDef(std::move(floorReplacementShapeDef));
	const VoxelTextureDefID floorReplacementVoxelTextureDefID = chunk.addTextureDef(std::move(floorReplacementTextureDef));
	const VoxelShadingDefID floorReplacementVoxelShadingDefID = chunk.addShadingDef(std::move(floorReplacementShadingDef));
	const VoxelTraitsDefID floorReplacementVoxelTraitsDefID = chunk.addTraitsDef(std::move(floorReplacementTraitsDef));
	chunk.setFloorReplacementShapeDefID(floorReplacementVoxelShapeDefID);
	chunk.setFloorReplacementTextureDefID(floorReplacementVoxelTextureDefID);
	chunk.setFloorReplacementShadingDefID(floorReplacementVoxelShadingDefID);
	chunk.setFloorReplacementTraitsDefID(floorReplacementVoxelTraitsDefID);

	// Reuse chasm definitions across all chunks.
	const LevelVoxelChasmDefID levelFloorReplacementChasmDefID = levelDefinition.getFloorReplacementChasmDefID();
	VoxelChasmDefinition floorReplacementChasmDef = levelInfoDefinition.getChasmDef(levelFloorReplacementChasmDefID);	
	VoxelChasmDefID floorReplacementChasmDefID = this->findChasmDef(floorReplacementChasmDef);
	if (floorReplacementChasmDefID < 0)
	{
		floorReplacementChasmDefID = this->addChasmDef(std::move(floorReplacementChasmDef));
	}

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

	for (WEInt z = startZ; z < endZ; z++)
	{
		for (int y = startY; y < endY; y++)
		{
			for (SNInt x = startX; x < endX; x++)
			{
				const VoxelInt3 chunkVoxel(x - startX, y - startY, z - startZ);
				const LevelVoxelShapeDefID levelVoxelShapeDefID = levelDefinition.getVoxelShapeID(x, y, z);
				const LevelVoxelTextureDefID levelVoxelTextureDefID = levelDefinition.getVoxelTextureID(x, y, z);
				const LevelVoxelShadingDefID levelVoxelShadingDefID = levelDefinition.getVoxelShadingID(x, y, z);
				const LevelVoxelTraitsDefID levelVoxelTraitsDefID = levelDefinition.getVoxelTraitsID(x, y, z);
				const VoxelShapeDefID voxelShapeDefID = LevelVoxelShapeDefIdToChunkVoxelShapeDefID(levelVoxelShapeDefID);
				const VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelShadingDefID voxelShadingDefID = LevelVoxelShadingDefIdToChunkVoxelShadingDefID(levelVoxelShadingDefID);
				const VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setShapeDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelShapeDefID);
				chunk.setTextureDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelTextureDefID);
				chunk.setShadingDefID(chunkVoxel.x, chunkVoxel.y, chunkVoxel.z, voxelShadingDefID);
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
		const LevelTransitionPlacementDefinition &placementDef = levelDefinition.getTransitionPlacementDef(i);
		const TransitionDefinition &transitionDef = levelInfoDefinition.getTransitionDef(placementDef.id);

		std::optional<VoxelTransitionDefID> transitionDefID;
		for (const WorldInt3 position : placementDef.positions)
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
		const LevelTriggerPlacementDefinition &placementDef = levelDefinition.getTriggerPlacementDef(i);
		const VoxelTriggerDefinition &triggerDef = levelInfoDefinition.getTriggerDef(placementDef.id);

		std::optional<VoxelTriggerDefID> triggerDefID;
		for (const WorldInt3 position : placementDef.positions)
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
		const LevelLockPlacementDefinition &placementDef = levelDefinition.getLockPlacementDef(i);
		const LockDefinition &lockDef = levelInfoDefinition.getLockDef(placementDef.id);

		std::optional<VoxelLockDefID> lockDefID;
		for (const WorldInt3 position : placementDef.positions)
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
		const LevelBuildingNamePlacementDefinition &placementDef = levelDefinition.getBuildingNamePlacementDef(i);
		const std::string &buildingName = levelInfoDefinition.getBuildingName(placementDef.id);

		std::optional<VoxelBuildingNameID> buildingNameID;
		for (const WorldInt3 position : placementDef.positions)
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
		const LevelDoorPlacementDefinition &placementDef = levelDefinition.getDoorPlacementDef(i);
		const VoxelDoorDefinition &doorDef = levelInfoDefinition.getDoorDef(placementDef.id);

		std::optional<VoxelDoorDefID> doorDefID;
		for (const WorldInt3 position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				if (!doorDefID.has_value())
				{
					doorDefID = chunk.addDoorDef(VoxelDoorDefinition(doorDef));
				}

				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addDoorDefPosition(*doorDefID, voxel);
			}
		}
	}

	// Add chasm definitions.
	for (int i = 0; i < levelDefinition.getChasmPlacementDefCount(); i++)
	{
		const LevelChasmPlacementDefinition &placementDef = levelDefinition.getChasmPlacementDef(i);
		const VoxelChasmDefinition &chasmDef = levelInfoDefinition.getChasmDef(placementDef.id);
		
		VoxelChasmDefID chasmDefID = this->findChasmDef(chasmDef);
		if (chasmDefID < 0)
		{
			chasmDefID = this->addChasmDef(VoxelChasmDefinition(chasmDef));
		}

		for (const WorldInt3 position : placementDef.positions)
		{
			if (ChunkUtils::IsInWritingRange(position, startX, endX, startY, endY, startZ, endZ))
			{
				const VoxelInt3 voxel = ChunkUtils::MakeChunkVoxelFromLevel(position, startX, startY, startZ);
				chunk.addChasmDefPosition(chasmDefID, voxel);
			}
		}
	}
}

void VoxelChunkManager::populateWildChunkBuildingNames(VoxelChunk &chunk,
	const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo, const LevelInfoDefinition &levelInfoDefinition)
{
	// Cache of level building names that have been added to the chunk.
	std::unordered_map<LevelVoxelBuildingNameID, VoxelBuildingNameID> buildingNameIDs;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.height; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				VoxelTransitionDefID transitionDefID;
				if (!chunk.tryGetTransitionDefID(x, y, z, &transitionDefID))
				{
					continue;
				}

				const TransitionDefinition &transitionDef = chunk.getTransitionDef(transitionDefID);
				if (transitionDef.type != TransitionType::EnterInterior)
				{
					continue;
				}

				const InteriorEntranceTransitionDefinition &interiorEntranceDef = transitionDef.interiorEntrance;
				const MapGeneration::InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;
				const ArenaInteriorType interiorType = interiorGenInfo.interiorType;

				LevelVoxelBuildingNameID buildingNameID;
				if (!buildingNameInfo.tryGetBuildingNameID(interiorType, &buildingNameID))
				{
					continue;
				}

				const std::string &buildingName = levelInfoDefinition.getBuildingName(buildingNameID);
				VoxelBuildingNameID chunkBuildingNameID;
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

	const ChunkInt2 chunkPos = chunk.position;
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.height; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelShapeDefID voxelShapeDefID = chunk.getShapeDefID(x, y, z);
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
				VoxelShapeDefID northVoxelShapeDefID, eastVoxelShapeDefID, southVoxelShapeDefID, westVoxelShapeDefID;
				this->getAdjacentVoxelShapeDefIDs(coord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
					&northVoxelShapeDefID, &eastVoxelShapeDefID, &southVoxelShapeDefID, &westVoxelShapeDefID);

				auto isFaceActive = [this](const std::optional<int> &chunkIndex, VoxelShapeDefID shapeDefID)
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

	const ChunkInt2 chunkPos = chunk.position;
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.height; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				VoxelDoorDefID doorDefID;
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

	chunk.init(chunkPos, levelHeight);
	this->populateChunkVoxelDefs(chunk, levelDef, levelInfoDef);

	// Populate all or part of the chunk from a level definition depending on the world type.
	const MapType mapType = mapSubDef.type;
	if (mapType == MapType::Interior)
	{
		// @todo: populate chunk entirely from default empty chunk (fast copy).
		// - probably get from MapDefinitionInterior eventually.
		constexpr VoxelShapeDefID floorVoxelShapeDefID = 2;
		const VoxelShapeDefID ceilingVoxelShapeDefID = [&levelInfoDef]()
		{
			for (int i = 0; i < levelInfoDef.getVoxelTraitsDefCount(); i++)
			{
				// @todo: from the looks of this, the engine needs to care about the concept of a "ceiling"?
				const VoxelTraitsDefinition &voxelTraitsDef = levelInfoDef.getVoxelTraitsDef(i);
				if (voxelTraitsDef.type == ArenaVoxelType::Ceiling)
				{
					return LevelVoxelShapeDefIdToChunkVoxelShapeDefID(i); // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.
				}
			}

			// No ceiling found, use air instead.
			return VoxelChunk::AIR_SHAPE_DEF_ID;
		}();

		constexpr VoxelTextureDefID floorVoxelTextureDefID = floorVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> texture def ID mapping.
		const VoxelTextureDefID ceilingVoxelTextureDefID = ceilingVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> texture def ID mapping.

		constexpr VoxelShadingDefID floorVoxelShadingDefID = floorVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> shading def ID mapping.
		const VoxelShadingDefID ceilingVoxelShadingDefID = ceilingVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> shading def ID mapping.

		constexpr VoxelTraitsDefID floorVoxelTraitsDefID = floorVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.
		const VoxelTraitsDefID ceilingVoxelTraitsDefID = ceilingVoxelShapeDefID; // @todo: this is probably brittle; can't assume shape def ID -> traits def ID mapping.

		const int chunkHeight = chunk.height;
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				chunk.setShapeDefID(x, 0, z, floorVoxelShapeDefID);
				chunk.setTextureDefID(x, 0, z, floorVoxelTextureDefID);
				chunk.setShadingDefID(x, 0, z, floorVoxelShadingDefID);
				chunk.setTraitsDefID(x, 0, z, floorVoxelTraitsDefID);

				if (chunkHeight > 2)
				{
					chunk.setShapeDefID(x, 2, z, ceilingVoxelShapeDefID);
					chunk.setTextureDefID(x, 2, z, ceilingVoxelTextureDefID);
					chunk.setShadingDefID(x, 2, z, ceilingVoxelShadingDefID);
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

				const LevelVoxelShapeDefID levelVoxelShapeDefID = levelDef.getVoxelShapeID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelVoxelTextureDefID levelVoxelTextureDefID = levelDef.getVoxelTextureID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelVoxelShadingDefID levelVoxelShadingDefID = levelDef.getVoxelShadingID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const LevelVoxelTraitsDefID levelVoxelTraitsDefID = levelDef.getVoxelTraitsID(wrappedLevelVoxel.x, 0, wrappedLevelVoxel.y);
				const VoxelShapeDefID voxelShapeDefID = LevelVoxelShapeDefIdToChunkVoxelShapeDefID(levelVoxelShapeDefID);
				const VoxelTextureDefID voxelTextureDefID = LevelVoxelTextureDefIdToChunkVoxelTextureDefID(levelVoxelTextureDefID);
				const VoxelShadingDefID voxelShadingDefID = LevelVoxelShadingDefIdToChunkVoxelShadingDefID(levelVoxelShadingDefID);
				const VoxelTraitsDefID voxelTraitsDefID = LevelVoxelTraitsDefIdToChunkVoxelTraitsDefID(levelVoxelTraitsDefID);
				chunk.setShapeDefID(x, 0, z, voxelShapeDefID);
				chunk.setTextureDefID(x, 0, z, voxelTextureDefID);
				chunk.setShadingDefID(x, 0, z, voxelShadingDefID);
				chunk.setTraitsDefID(x, 0, z, voxelTraitsDefID);
			}
		}

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelWidth, levelDepth))
		{
			// Populate chunk from the part of the level it overlaps.
			const WorldInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkVoxels(chunk, levelDef, levelOffset);
			this->populateChunkDecorators(chunk, levelDef, levelInfoDef, levelOffset);
			this->populateChunkDoorVisibilityInsts(chunk);
		}

		// Need out-of-city-bounds chasms to be defined as well.
		// @todo organize the decorator loops better so the tryGetChasmDefID() check here isn't needed and we don't try to double-add chasm def positions
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(x, 0, z);
				const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
				if (voxelTraitsDef.type == ArenaVoxelType::Chasm)
				{
					VoxelChasmDefID dummyChasmDefID;
					if (!chunk.tryGetChasmDefID(x, 0, z, &dummyChasmDefID))
					{
						const VoxelChasmDefID chasmDefID = chunk.getFloorReplacementChasmDefID();
						chunk.addChasmDefPosition(chasmDefID, VoxelInt3(x, 0, z));
					}					
				}
			}
		}

		this->populateChunkChasmInsts(chunk);
	}
	else if (mapType == MapType::Wilderness)
	{
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
	const CoordInt3 coord(chunk.position, voxel);
	auto getChasmFaces = [this, &coord](bool *outNorth, bool *outEast, bool *outSouth, bool *outWest)
	{
		auto getChasmFace = [this](const std::optional<int> &chunkIndex, VoxelShapeDefID shapeDefID)
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
		VoxelShapeDefID northDefID, eastDefID, southDefID, westDefID;
		this->getAdjacentVoxelShapeDefIDs(coord, &outNorthChunkIndex, &outEastChunkIndex, &outSouthChunkIndex,
			&outWestChunkIndex, &northDefID, &eastDefID, &southDefID, &westDefID);

		*outNorth = getChasmFace(outNorthChunkIndex, northDefID);
		*outEast = getChasmFace(outEastChunkIndex, eastDefID);
		*outSouth = getChasmFace(outSouthChunkIndex, southDefID);
		*outWest = getChasmFace(outWestChunkIndex, westDefID);
	};

	const VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(x, y, z);
	const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
	if (voxelTraitsDef.type == ArenaVoxelType::Chasm)
	{
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
			// No instance yet. Add a new voxel instance.
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
	const ChunkInt2 chunkPos = chunk.position;
	const ChunkInt2 playerChunkPos = playerCoord.chunk;
	const VoxelInt2 playerVoxelXZ = VoxelUtils::pointToVoxel(playerCoord.point.getXZ());

	for (VoxelDoorVisibilityInstance &visInst : chunk.getDoorVisibilityInsts())
	{
		const VoxelInt3 doorVoxel(visInst.x, visInst.y, visInst.z);
		const bool isCameraNorthInclusive = (playerChunkPos.x < chunkPos.x) || ((playerChunkPos.x == chunkPos.x) && (playerVoxelXZ.x <= doorVoxel.x));
		const bool isCameraEastInclusive = (playerChunkPos.y < chunkPos.y) || ((playerChunkPos.y == chunkPos.y) && (playerVoxelXZ.y <= doorVoxel.z));

		const CoordInt3 doorVoxelCoord(chunkPos, doorVoxel);
		std::optional<int> northChunkIndex, eastChunkIndex, southChunkIndex, westChunkIndex;
		VoxelShapeDefID northVoxelShapeDefID, eastVoxelShapeDefID, southVoxelShapeDefID, westVoxelShapeDefID;
		this->getAdjacentVoxelShapeDefIDs(doorVoxelCoord, &northChunkIndex, &eastChunkIndex, &southChunkIndex, &westChunkIndex,
			&northVoxelShapeDefID, &eastVoxelShapeDefID, &southVoxelShapeDefID, &westVoxelShapeDefID);

		auto isVoxelValidForDoorFace = [this](const std::optional<int> &chunkIndex, VoxelShapeDefID shapeDefID)
		{
			if (!chunkIndex.has_value())
			{
				return true;
			}

			const VoxelChunk &voxelChunk = this->getChunkAtIndex(*chunkIndex);
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(shapeDefID);
			const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
			return voxelMeshDef.isEmpty() || voxelShapeDef.allowsAdjacentDoorFaces;
		};

		const bool isNorthValid = isVoxelValidForDoorFace(northChunkIndex, northVoxelShapeDefID);
		const bool isEastValid = isVoxelValidForDoorFace(eastChunkIndex, eastVoxelShapeDefID);
		const bool isSouthValid = isVoxelValidForDoorFace(southChunkIndex, southVoxelShapeDefID);
		const bool isWestValid = isVoxelValidForDoorFace(westChunkIndex, westVoxelShapeDefID);

		visInst.update(isCameraNorthInclusive, isCameraEastInclusive, isNorthValid, isEastValid, isSouthValid, isWestValid);
		chunk.addDirtyDoorVisInstPosition(doorVoxel); // @todo why is this dirtying every frame?
	}
}

void VoxelChunkManager::update(double dt, BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const CoordDouble3 &playerCoord, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
	const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs, BufferView<const int> levelInfoDefIndices,
	BufferView<const LevelInfoDefinition> levelInfoDefs, double ceilingScale, AudioManager &audioManager)
{
	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	const MapType mapType = mapSubDef.type;
	for (const ChunkInt2 chunkPos : newChunkPositions)
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
		chunkPtr->updateDoorAnimInsts(dt, playerCoord, ceilingScale, audioManager);
		chunkPtr->updateFadeAnimInsts(dt);
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

		for (const VoxelInt3 dirtyChasmWallPos : cachedDirtyChasmWallPositions)
		{
			const CoordInt3 coord(chunkPtr->position, dirtyChasmWallPos);
			const CoordInt3 adjacentCoords[] =
			{
				VoxelUtils::getCoordWithOffset(coord, VoxelUtils::North),
				VoxelUtils::getCoordWithOffset(coord, VoxelUtils::East),
				VoxelUtils::getCoordWithOffset(coord, VoxelUtils::South),
				VoxelUtils::getCoordWithOffset(coord, VoxelUtils::West)
			};

			for (const CoordInt3 adjacentCoord : adjacentCoords)
			{
				const std::optional<int> adjacentChunkIndex = this->tryGetChunkIndex(adjacentCoord.chunk);
				if (adjacentChunkIndex.has_value())
				{
					VoxelChunk &adjacentChunk = this->getChunkAtIndex(*adjacentChunkIndex);
					const VoxelInt3 adjacentVoxel = adjacentCoord.voxel;
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

		for (const VoxelInt3 chasmWallPos : chunk.getDirtyChasmWallInstPositions())
		{
			this->updateChasmWallInst(chunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		// North and south sides.
		const int chunkHeight = chunk.height;
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

void VoxelChunkManager::clear()
{
	this->chasmDefs.clear();
	this->recycleAllChunks();
}
