#include <algorithm>
#include <type_traits>

#include "VoxelChunk.h"
#include "../Audio/AudioManager.h"

#include "components/debug/Debug.h"

VoxelChunk::VoxelChunk()
{
	this->floorReplacementShapeDefID = -1;
	this->floorReplacementTextureDefID = -1;
	this->floorReplacementShadingDefID = -1;
	this->floorReplacementTraitsDefID = -1;
	this->floorReplacementChasmDefID = -1;
}

void VoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first voxel definition (air) be usable immediately. All default voxel IDs can safely point to it.
	this->shapeDefs.emplace_back(VoxelShapeDefinition());
	this->textureDefs.emplace_back(VoxelTextureDefinition());
	this->shadingDefs.emplace_back(VoxelShadingDefinition());
	this->traitsDefs.emplace_back(VoxelTraitsDefinition());

	// Set all voxels to air.
	this->shapeDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->shapeDefIDs.fill(VoxelChunk::AIR_SHAPE_DEF_ID);

	this->textureDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->textureDefIDs.fill(VoxelChunk::AIR_TEXTURE_DEF_ID);

	this->shadingDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->shadingDefIDs.fill(VoxelChunk::AIR_SHADING_DEF_ID);

	this->traitsDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->traitsDefIDs.fill(VoxelChunk::AIR_TRAITS_DEF_ID);

	this->dirtyVoxelTypes.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->dirtyVoxelTypes.fill(static_cast<VoxelDirtyType>(0));
	this->dirtyShapeDefPositions.reserve(Chunk::WIDTH * height * Chunk::DEPTH);
}

void VoxelChunk::getAdjacentShapeDefIDs(const VoxelInt3 &voxel, VoxelShapeDefID *outNorthID, VoxelShapeDefID *outEastID,
	VoxelShapeDefID *outSouthID, VoxelShapeDefID *outWestID) const
{
	this->getAdjacentIDsInternal<VoxelShapeDefID>(voxel, this->shapeDefIDs, VoxelChunk::AIR_SHAPE_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID, VoxelTextureDefID *outEastID,
	VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID) const
{
	this->getAdjacentIDsInternal<VoxelTextureDefID>(voxel, this->textureDefIDs, VoxelChunk::AIR_TEXTURE_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentShadingDefIDs(const VoxelInt3 &voxel, VoxelShadingDefID *outNorthID, VoxelShadingDefID *outEastID,
	VoxelShadingDefID *outSouthID, VoxelShadingDefID *outWestID) const
{
	this->getAdjacentIDsInternal<VoxelShadingDefID>(voxel, this->shadingDefIDs, VoxelChunk::AIR_SHADING_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID, VoxelTraitsDefID *outEastID,
	VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID) const
{
	this->getAdjacentIDsInternal<VoxelTraitsDefID>(voxel, this->traitsDefIDs, VoxelChunk::AIR_TRAITS_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

bool VoxelChunk::tryGetTransitionDefID(SNInt x, int y, WEInt z, VoxelTransitionDefID *outID) const
{
	const auto iter = this->transitionDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->transitionDefIndices.end())
	{
		const VoxelTransitionDefID id = iter->second;
		DebugAssertIndex(this->transitionDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetTriggerDefID(SNInt x, int y, WEInt z, VoxelTriggerDefID *outID) const
{
	const auto iter = this->triggerDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->triggerDefIndices.end())
	{
		const VoxelTriggerDefID id = iter->second;
		DebugAssertIndex(this->triggerDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetLockDefID(SNInt x, int y, WEInt z, VoxelLockDefID *outID) const
{
	const auto iter = this->lockDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->lockDefIndices.end())
	{
		const VoxelLockDefID id = iter->second;
		DebugAssertIndex(this->lockDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetBuildingNameID(SNInt x, int y, WEInt z, VoxelBuildingNameID *outID) const
{
	const auto iter = this->buildingNameIndices.find(VoxelInt3(x, y, z));
	if (iter != this->buildingNameIndices.end())
	{
		const VoxelBuildingNameID id = iter->second;
		DebugAssertIndex(this->buildingNames, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetDoorDefID(SNInt x, int y, WEInt z, VoxelDoorDefID *outID) const
{
	const auto iter = this->doorDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->doorDefIndices.end())
	{
		const VoxelDoorDefID id = iter->second;
		DebugAssertIndex(this->doorDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetChasmDefID(SNInt x, int y, WEInt z, VoxelChasmDefID *outID) const
{
	const auto iter = this->chasmDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->chasmDefIndices.end())
	{
		const VoxelChasmDefID id = iter->second;
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetDoorAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const
{
	const auto iter = std::find_if(this->doorAnimInsts.begin(), this->doorAnimInsts.end(),
		[x, y, z](const VoxelDoorAnimationInstance &animInst)
	{
		return (animInst.x == x) && (animInst.y == y) && (animInst.z == z);
	});

	if (iter != this->doorAnimInsts.end())
	{
		*outIndex = static_cast<int>(std::distance(this->doorAnimInsts.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetFadeAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->fadeAnimInsts.size()); i++)
	{
		const VoxelFadeAnimationInstance &inst = this->fadeAnimInsts[i];
		if ((inst.x == x) && (inst.y == y) && (inst.z == z))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool VoxelChunk::tryGetChasmWallInstIndex(SNInt x, int y, WEInt z, int *outIndex) const
{
	const auto iter = std::find_if(this->chasmWallInsts.begin(), this->chasmWallInsts.end(),
		[x, y, z](const VoxelChasmWallInstance &inst)
	{
		return (inst.x == x) && (inst.y == y) && (inst.z == z);
	});

	if (iter != this->chasmWallInsts.end())
	{
		*outIndex = static_cast<int>(std::distance(this->chasmWallInsts.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetDoorVisibilityInstIndex(SNInt x, int y, WEInt z, int *outIndex) const
{
	const auto iter = std::find_if(this->doorVisInsts.begin(), this->doorVisInsts.end(),
		[x, y, z](const VoxelDoorVisibilityInstance &inst)
	{
		return (inst.x == x) && (inst.y == y) && (inst.z == z);
	});

	if (iter != this->doorVisInsts.end())
	{
		*outIndex = static_cast<int>(std::distance(this->doorVisInsts.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetTriggerInstIndex(SNInt x, int y, WEInt z, int *outIndex) const
{
	const auto iter = std::find_if(this->triggerInsts.begin(), this->triggerInsts.end(),
		[x, y, z](const VoxelTriggerInstance &inst)
	{
		return (inst.x == x) && (inst.y == y) && (inst.z == z);
	});

	if (iter != this->triggerInsts.end())
	{
		*outIndex = static_cast<int>(std::distance(this->triggerInsts.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

void VoxelChunk::setShapeDefID(SNInt x, int y, WEInt z, VoxelShapeDefID id)
{
	this->shapeDefIDs.set(x, y, z, id);
	this->setShapeDefDirty(x, y, z);
}

void VoxelChunk::setTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id)
{
	this->textureDefIDs.set(x, y, z, id);
}

void VoxelChunk::setShadingDefID(SNInt x, int y, WEInt z, VoxelShadingDefID id)
{
	this->shadingDefIDs.set(x, y, z, id);
}

void VoxelChunk::setTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id)
{
	this->traitsDefIDs.set(x, y, z, id);
}

VoxelShapeDefID VoxelChunk::addShapeDef(VoxelShapeDefinition &&voxelShapeDef)
{
	const VoxelShapeDefID id = static_cast<VoxelShapeDefID>(this->shapeDefs.size());
	this->shapeDefs.emplace_back(std::move(voxelShapeDef));
	return id;
}

VoxelTextureDefID VoxelChunk::addTextureDef(VoxelTextureDefinition &&voxelTextureDef)
{
	const VoxelTextureDefID id = static_cast<VoxelTextureDefID>(this->textureDefs.size());
	this->textureDefs.emplace_back(std::move(voxelTextureDef));
	return id;
}

VoxelShadingDefID VoxelChunk::addShadingDef(VoxelShadingDefinition &&voxelShadingDef)
{
	const VoxelShadingDefID id = static_cast<VoxelShadingDefID>(this->shadingDefs.size());
	this->shadingDefs.emplace_back(std::move(voxelShadingDef));
	return id;
}

VoxelTraitsDefID VoxelChunk::addTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef)
{
	const VoxelTraitsDefID id = static_cast<VoxelTraitsDefID>(this->traitsDefs.size());
	this->traitsDefs.emplace_back(std::move(voxelTraitsDef));
	return id;
}

VoxelTransitionDefID VoxelChunk::addTransitionDef(TransitionDefinition &&transition)
{
	const VoxelTransitionDefID id = static_cast<int>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(transition));
	return id;
}

VoxelTriggerDefID VoxelChunk::addTriggerDef(VoxelTriggerDefinition &&trigger)
{
	const VoxelTriggerDefID id = static_cast<int>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(trigger));
	return id;
}

VoxelLockDefID VoxelChunk::addLockDef(LockDefinition &&lock)
{
	const VoxelLockDefID id = static_cast<int>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(lock));
	return id;
}

VoxelBuildingNameID VoxelChunk::addBuildingName(std::string &&buildingName)
{
	const VoxelBuildingNameID id = static_cast<int>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(buildingName));
	return id;
}

VoxelDoorDefID VoxelChunk::addDoorDef(VoxelDoorDefinition &&door)
{
	const VoxelDoorDefID id = static_cast<int>(this->doorDefs.size());
	this->doorDefs.emplace_back(std::move(door));
	return id;
}

void VoxelChunk::addTransitionDefPosition(VoxelTransitionDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->transitionDefIndices.find(voxel) == this->transitionDefIndices.end());
	this->transitionDefIndices.emplace(voxel, id);
}

void VoxelChunk::addTriggerDefPosition(VoxelTriggerDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->triggerDefIndices.find(voxel) == this->triggerDefIndices.end());
	this->triggerDefIndices.emplace(voxel, id);
}

void VoxelChunk::addLockDefPosition(VoxelLockDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->lockDefIndices.find(voxel) == this->lockDefIndices.end());
	this->lockDefIndices.emplace(voxel, id);
}

void VoxelChunk::addBuildingNamePosition(VoxelBuildingNameID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->buildingNameIndices.find(voxel) == this->buildingNameIndices.end());
	this->buildingNameIndices.emplace(voxel, id);
}

void VoxelChunk::addDoorDefPosition(VoxelDoorDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->doorDefIndices.find(voxel) == this->doorDefIndices.end());
	this->doorDefIndices.emplace(voxel, id);
}

void VoxelChunk::addChasmDefPosition(VoxelChasmDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->chasmDefIndices.find(voxel) == this->chasmDefIndices.end());
	this->chasmDefIndices.emplace(voxel, id);
}

void VoxelChunk::removeChasmWallInst(const VoxelInt3 &voxel)
{
	for (int i = 0; i < static_cast<int>(this->chasmWallInsts.size()); i++)
	{
		const VoxelChasmWallInstance &chasmWallInst = this->chasmWallInsts[i];
		if ((chasmWallInst.x == voxel.x) && (chasmWallInst.y == voxel.y) && (chasmWallInst.z == voxel.z))
		{
			this->chasmWallInsts.erase(this->chasmWallInsts.begin() + i);
			break;
		}
	}
}

void VoxelChunk::trySetVoxelDirtyInternal(SNInt x, int y, WEInt z, std::vector<VoxelInt3> &dirtyPositions, VoxelDirtyType dirtyType)
{
	static_assert(std::is_same_v<std::underlying_type_t<VoxelDirtyType>, uint8_t>);

	const VoxelDirtyType prevDirtyType = this->dirtyVoxelTypes.get(x, y, z);
	const uint8_t prevFlags = static_cast<uint8_t>(prevDirtyType);
	const uint8_t flags = static_cast<uint8_t>(dirtyType);

	if ((prevFlags & flags) == 0)
	{
		dirtyPositions.emplace_back(VoxelInt3(x, y, z));

		const VoxelDirtyType sumDirtyType = static_cast<VoxelDirtyType>(prevFlags | flags);
		this->dirtyVoxelTypes.set(x, y, z, sumDirtyType);
	}
}

void VoxelChunk::setShapeDefDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyShapeDefPositions, VoxelDirtyType::ShapeDefinition);
}

void VoxelChunk::setFaceActivationDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyFaceActivationPositions, VoxelDirtyType::FaceActivation);
}

void VoxelChunk::setDoorAnimInstDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyDoorAnimInstPositions, VoxelDirtyType::DoorAnimation);
}

void VoxelChunk::setDoorVisInstDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyDoorVisInstPositions, VoxelDirtyType::DoorVisibility);
}

void VoxelChunk::setFadeAnimInstDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyFadeAnimInstPositions, VoxelDirtyType::FadeAnimation);
}

void VoxelChunk::updateDoorAnimInsts(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager)
{
	const ChunkInt2 chunkPos = this->position;

	for (int i = static_cast<int>(this->doorAnimInsts.size()) - 1; i >= 0; i--)
	{
		VoxelDoorAnimationInstance &animInst = this->doorAnimInsts[i];
		animInst.update(dt);

		const VoxelInt3 voxel(animInst.x, animInst.y, animInst.z);
		if (animInst.stateType != VoxelDoorAnimationStateType::Closed)
		{
			if (animInst.stateType != VoxelDoorAnimationStateType::Closing)
			{
				// If the player is far enough away, set the door to closing.
				const CoordDouble3 voxelCoord(chunkPos, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
				const VoxelDouble3 diff = playerCoord - voxelCoord;

				constexpr double closeDistSqr = ArenaLevelUtils::DOOR_CLOSE_DISTANCE * ArenaLevelUtils::DOOR_CLOSE_DISTANCE;
				const double distSqr = diff.lengthSquared();

				if (distSqr >= closeDistSqr)
				{
					animInst.setStateType(VoxelDoorAnimationStateType::Closing);

					// Play closing sound if it's defined for the door.
					VoxelDoorDefID doorDefID;
					if (!this->tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
					{
						DebugCrash("Expected door def ID to exist.");
					}

					const VoxelDoorDefinition &doorDef = this->doorDefs[doorDefID];
					const VoxelDoorCloseSoundDefinition &closeSoundDef = doorDef.closeSoundDef;
					if (closeSoundDef.closeType == VoxelDoorCloseType::OnClosing)
					{
						const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(voxelCoord);
						audioManager.playSound(closeSoundDef.soundFilename.c_str(), absoluteSoundPosition);
					}
				}
			}
		}
		else
		{
			// Play closed sound if it's defined for the door.
			VoxelDoorDefID doorDefID;
			if (!this->tryGetDoorDefID(animInst.x, animInst.y, animInst.z, &doorDefID))
			{
				DebugCrash("Expected door def ID to exist.");
			}

			const VoxelDoorDefinition &doorDef = this->doorDefs[doorDefID];
			const VoxelDoorCloseSoundDefinition &closeSoundDef = doorDef.closeSoundDef;
			if (closeSoundDef.closeType == VoxelDoorCloseType::OnClosed)
			{
				const CoordDouble3 soundCoord(chunkPos, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
				const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
				audioManager.playSound(closeSoundDef.soundFilename.c_str(), absoluteSoundPosition);
			}

			this->destroyedDoorAnimInsts.emplace_back(voxel);
		}

		this->setDoorAnimInstDirty(voxel.x, voxel.y, voxel.z);
	}
}

void VoxelChunk::updateFadeAnimInsts(double dt)
{
	for (int i = static_cast<int>(this->fadeAnimInsts.size()) - 1; i >= 0; i--)
	{
		VoxelFadeAnimationInstance &animInst = this->fadeAnimInsts[i];
		animInst.update(dt);

		const VoxelInt3 voxel(animInst.x, animInst.y, animInst.z);
		if (animInst.isDoneFading())
		{
			const VoxelTraitsDefID voxelTraitsDefID = this->traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = this->traitsDefs[voxelTraitsDefID];
			const bool shouldConvertToChasm = voxelTraitsDef.type == ArenaVoxelType::Floor;
			if (shouldConvertToChasm)
			{
				// Change to water chasm.
				this->setShapeDefID(voxel.x, voxel.y, voxel.z, this->floorReplacementShapeDefID);
				this->setTextureDefID(voxel.x, voxel.y, voxel.z, this->floorReplacementTextureDefID);
				this->setShadingDefID(voxel.x, voxel.y, voxel.z, this->floorReplacementShadingDefID);
				this->setTraitsDefID(voxel.x, voxel.y, voxel.z, this->floorReplacementTraitsDefID);
				this->chasmDefIndices.emplace(voxel, this->floorReplacementChasmDefID);
				this->setFaceActivationDirty(voxel.x, voxel.y, voxel.z);
			}
			else
			{
				// Air voxel.
				this->setShapeDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_SHAPE_DEF_ID);
				this->setTextureDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_TEXTURE_DEF_ID);
				this->setShadingDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_SHADING_DEF_ID);
				this->setTraitsDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_TRAITS_DEF_ID);

				auto tryEraseVoxelMapEntry = [&voxel](auto &map)
				{
					const auto mapIter = map.find(voxel);
					if (mapIter != map.end())
					{
						map.erase(mapIter);
					}
				};

				tryEraseVoxelMapEntry(this->transitionDefIndices);
				tryEraseVoxelMapEntry(this->triggerDefIndices);
				tryEraseVoxelMapEntry(this->lockDefIndices);
				tryEraseVoxelMapEntry(this->buildingNameIndices);
				tryEraseVoxelMapEntry(this->doorDefIndices);
				tryEraseVoxelMapEntry(this->chasmDefIndices);
			}

			// Set adjacent face activations dirty in case they became unblocked.
			const VoxelInt3 adjacentVoxels[] =
			{
				VoxelUtils::getVoxelWithOffset(voxel, VoxelInt3::UnitX),
				VoxelUtils::getVoxelWithOffset(voxel, -VoxelInt3::UnitX),
				VoxelUtils::getVoxelWithOffset(voxel, VoxelInt3::UnitY),
				VoxelUtils::getVoxelWithOffset(voxel, -VoxelInt3::UnitY),
				VoxelUtils::getVoxelWithOffset(voxel, VoxelInt3::UnitZ),
				VoxelUtils::getVoxelWithOffset(voxel, -VoxelInt3::UnitZ)
			};

			for (const VoxelInt3 adjacentVoxel : adjacentVoxels)
			{
				if (this->isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z))
				{
					this->setFaceActivationDirty(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
				}
			}

			this->destroyedFadeAnimInsts.emplace_back(voxel);
		}
		else
		{
			this->setFadeAnimInstDirty(voxel.x, voxel.y, voxel.z);
		}
	}
}

void VoxelChunk::endFrame()
{
	this->dirtyVoxelTypes.fill(static_cast<VoxelDirtyType>(0));
	this->dirtyShapeDefPositions.clear();
	this->dirtyFaceActivationPositions.clear();
	this->dirtyDoorAnimInstPositions.clear();
	this->dirtyDoorVisInstPositions.clear();
	this->dirtyFadeAnimInstPositions.clear();

	for (const VoxelInt3 position : this->destroyedDoorAnimInsts)
	{
		const auto iter = std::find_if(this->doorAnimInsts.begin(), this->doorAnimInsts.end(),
			[position](const VoxelDoorAnimationInstance &animInst)
		{
			return (animInst.x == position.x) && (animInst.y == position.y) && (animInst.z == position.z);
		});

		DebugAssert(iter != this->doorAnimInsts.end());
		this->doorAnimInsts.erase(iter);
	}

	this->destroyedDoorAnimInsts.clear();

	for (const VoxelInt3 position : this->destroyedFadeAnimInsts)
	{
		const auto iter = std::find_if(this->fadeAnimInsts.begin(), this->fadeAnimInsts.end(),
			[position](const VoxelFadeAnimationInstance &animInst)
		{
			return (animInst.x == position.x) && (animInst.y == position.y) && (animInst.z == position.z);
		});

		DebugAssert(iter != this->fadeAnimInsts.end());
		this->fadeAnimInsts.erase(iter);
	}

	this->destroyedFadeAnimInsts.clear();
}

void VoxelChunk::clear()
{
	Chunk::clear();
	this->shapeDefs.clear();
	this->textureDefs.clear();
	this->shadingDefs.clear();
	this->traitsDefs.clear();
	this->transitionDefs.clear();
	this->triggerDefs.clear();
	this->lockDefs.clear();
	this->buildingNames.clear();
	this->doorDefs.clear();
	this->shapeDefIDs.clear();
	this->textureDefIDs.clear();
	this->shadingDefIDs.clear();
	this->traitsDefIDs.clear();
	this->dirtyVoxelTypes.clear();
	this->dirtyShapeDefPositions.clear();
	this->dirtyFaceActivationPositions.clear();
	this->dirtyDoorAnimInstPositions.clear();
	this->dirtyDoorVisInstPositions.clear();
	this->dirtyFadeAnimInstPositions.clear();
	this->transitionDefIndices.clear();
	this->triggerDefIndices.clear();
	this->lockDefIndices.clear();
	this->buildingNameIndices.clear();
	this->doorDefIndices.clear();
	this->chasmDefIndices.clear();
	this->doorAnimInsts.clear();
	this->fadeAnimInsts.clear();
	this->chasmWallInsts.clear();
	this->doorVisInsts.clear();
	this->triggerInsts.clear();
	this->destroyedDoorAnimInsts.clear();
	this->destroyedFadeAnimInsts.clear();
}
