#include <algorithm>

#include "VoxelChunk.h"
#include "../Audio/AudioManager.h"

#include "components/debug/Debug.h"

void VoxelChunk::init(const ChunkInt2 &position, int height)
{
	// Set all voxels to air.
	this->voxelMeshDefIDs.init(VoxelChunk::WIDTH, height, VoxelChunk::DEPTH);
	this->voxelMeshDefIDs.fill(VoxelChunk::AIR_VOXEL_MESH_DEF_ID);

	this->voxelTextureDefIDs.init(VoxelChunk::WIDTH, height, VoxelChunk::DEPTH);
	this->voxelTextureDefIDs.fill(VoxelChunk::AIR_VOXEL_TEXTURE_DEF_ID);

	this->voxelTraitsDefIDs.init(VoxelChunk::WIDTH, height, VoxelChunk::DEPTH);
	this->voxelTraitsDefIDs.fill(VoxelChunk::AIR_VOXEL_TRAITS_DEF_ID);

	// Let the first voxel definition (air) be usable immediately. All default voxel IDs can safely point to it.
	this->voxelMeshDefs.emplace_back(VoxelMeshDefinition());
	this->voxelTextureDefs.emplace_back(VoxelTextureDefinition());
	this->voxelTraitsDefs.emplace_back(VoxelTraitsDefinition());

	this->dirtyVoxels.init(VoxelChunk::WIDTH, height, VoxelChunk::DEPTH);
	this->dirtyVoxels.fill(false);
	this->dirtyVoxelPositions.reserve(VoxelChunk::WIDTH * height * VoxelChunk::DEPTH);

	this->position = position;
}

const ChunkInt2 &VoxelChunk::getPosition() const
{
	return this->position;
}

bool VoxelChunk::isValidVoxel(SNInt x, int y, WEInt z) const
{
	return (x >= 0) && (x < VoxelChunk::WIDTH) && (y >= 0) && (y < this->getHeight()) && (z >= 0) && (z < VoxelChunk::DEPTH);
}

int VoxelChunk::getHeight() const
{
	DebugAssert(this->voxelMeshDefIDs.getHeight() == this->voxelTextureDefIDs.getHeight());
	DebugAssert(this->voxelMeshDefIDs.getHeight() == this->voxelTraitsDefIDs.getHeight());
	DebugAssert(this->voxelMeshDefIDs.getHeight() == this->dirtyVoxels.getHeight());
	return this->voxelMeshDefIDs.getHeight();
}

VoxelChunk::VoxelMeshDefID VoxelChunk::getVoxelMeshDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelMeshDefIDs.get(x, y, z);
}

VoxelChunk::VoxelTextureDefID VoxelChunk::getVoxelTextureDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelTextureDefIDs.get(x, y, z);
}

VoxelChunk::VoxelTraitsDefID VoxelChunk::getVoxelTraitsDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelTraitsDefIDs.get(x, y, z);
}

int VoxelChunk::getVoxelMeshDefCount() const
{
	return static_cast<int>(this->voxelMeshDefs.size());
}

int VoxelChunk::getVoxelTextureDefCount() const
{
	return static_cast<int>(this->voxelTextureDefs.size());
}

int VoxelChunk::getVoxelTraitsDefCount() const
{
	return static_cast<int>(this->voxelTraitsDefs.size());
}

const VoxelMeshDefinition &VoxelChunk::getVoxelMeshDef(int index) const
{
	DebugAssertIndex(this->voxelMeshDefs, index);
	return this->voxelMeshDefs[index];
}

const VoxelTextureDefinition &VoxelChunk::getVoxelTextureDef(int index) const
{
	DebugAssertIndex(this->voxelTextureDefs, index);
	return this->voxelTextureDefs[index];
}

const VoxelTraitsDefinition &VoxelChunk::getVoxelTraitsDef(int index) const
{
	DebugAssertIndex(this->voxelTraitsDefs, index);
	return this->voxelTraitsDefs[index];
}

int VoxelChunk::getDirtyVoxelCount() const
{
	return static_cast<int>(this->dirtyVoxelPositions.size());
}

const VoxelInt3 &VoxelChunk::getDirtyVoxel(int index) const
{
	DebugAssertIndex(this->dirtyVoxelPositions, index);
	return this->dirtyVoxelPositions[index];
}

int VoxelChunk::getVoxelInstCount() const
{
	return static_cast<int>(this->voxelInsts.size());
}

VoxelInstance &VoxelChunk::getVoxelInst(int index)
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

const VoxelInstance &VoxelChunk::getVoxelInst(int index) const
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

std::optional<int> VoxelChunk::tryGetVoxelInstIndex(const VoxelInt3 &voxel, VoxelInstance::Type type) const
{
	for (int i = 0; i < static_cast<int>(this->voxelInsts.size()); i++)
	{
		const VoxelInstance &inst = this->voxelInsts[i];
		if ((inst.getX() == voxel.x) && (inst.getY() == voxel.y) && (inst.getZ() == voxel.z) &&
			(inst.getType() == type))
		{
			return i;
		}
	}

	return std::nullopt;
}

VoxelInstance *VoxelChunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
{
	const std::optional<int> index = this->tryGetVoxelInstIndex(voxel, type);
	return index.has_value() ? &this->voxelInsts[*index] : nullptr;
}

const VoxelInstance *VoxelChunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type) const
{
	const std::optional<int> index = this->tryGetVoxelInstIndex(voxel, type);
	return index.has_value() ? &this->voxelInsts[*index] : nullptr;
}

bool VoxelChunk::tryGetTransitionDefID(SNInt x, int y, WEInt z, TransitionDefID *outID) const
{
	const auto iter = this->transitionDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->transitionDefIndices.end())
	{
		const TransitionDefID id = iter->second;
		DebugAssertIndex(this->transitionDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetTriggerDefID(SNInt x, int y, WEInt z, TriggerDefID *outID) const
{
	const auto iter = this->triggerDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->triggerDefIndices.end())
	{
		const TriggerDefID id = iter->second;
		DebugAssertIndex(this->triggerDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetLockDefID(SNInt x, int y, WEInt z, LockDefID *outID) const
{
	const auto iter = this->lockDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->lockDefIndices.end())
	{
		const LockDefID id = iter->second;
		DebugAssertIndex(this->lockDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetBuildingNameID(SNInt x, int y, WEInt z, BuildingNameID *outID) const
{
	const auto iter = this->buildingNameIndices.find(VoxelInt3(x, y, z));
	if (iter != this->buildingNameIndices.end())
	{
		const BuildingNameID id = iter->second;
		DebugAssertIndex(this->buildingNames, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetDoorDefID(SNInt x, int y, WEInt z, DoorDefID *outID) const
{
	const auto iter = this->doorDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->doorDefIndices.end())
	{
		const DoorDefID id = iter->second;
		DebugAssertIndex(this->doorDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

bool VoxelChunk::tryGetChasmDefID(SNInt x, int y, WEInt z, ChasmDefID *outID) const
{
	const auto iter = this->chasmDefIndices.find(VoxelInt3(x, y, z));
	if (iter != this->chasmDefIndices.end())
	{
		const ChasmDefID id = iter->second;
		DebugAssertIndex(this->chasmDefs, id);
		*outID = id;
		return true;
	}
	else
	{
		return false;
	}
}

int VoxelChunk::getTransitionDefCount() const
{
	return static_cast<int>(this->transitionDefs.size());
}

int VoxelChunk::getTriggerDefCount() const
{
	return static_cast<int>(this->triggerDefs.size());
}

int VoxelChunk::getLockDefCount() const
{
	return static_cast<int>(this->lockDefs.size());
}

int VoxelChunk::getBuildingNameDefCount() const
{
	return static_cast<int>(this->buildingNames.size());
}

int VoxelChunk::getDoorDefCount() const
{
	return static_cast<int>(this->doorDefs.size());
}

int VoxelChunk::getChasmDefCount() const
{
	return static_cast<int>(this->chasmDefs.size());
}

const TransitionDefinition &VoxelChunk::getTransitionDef(TransitionDefID id) const
{
	DebugAssertIndex(this->transitionDefs, id);
	return this->transitionDefs[id];
}

const TriggerDefinition &VoxelChunk::getTriggerDef(TriggerDefID id) const
{
	DebugAssertIndex(this->triggerDefs, id);
	return this->triggerDefs[id];
}

const LockDefinition &VoxelChunk::getLockDef(LockDefID id) const
{
	DebugAssertIndex(this->lockDefs, id);
	return this->lockDefs[id];
}

const std::string &VoxelChunk::getBuildingName(BuildingNameID id) const
{
	DebugAssertIndex(this->buildingNames, id);
	return this->buildingNames[id];
}

const DoorDefinition &VoxelChunk::getDoorDef(DoorDefID id) const
{
	DebugAssertIndex(this->doorDefs, id);
	return this->doorDefs[id];
}

const ChasmDefinition &VoxelChunk::getChasmDef(ChasmDefID id) const
{
	DebugAssertIndex(this->chasmDefs, id);
	return this->chasmDefs[id];
}

template <typename VoxelIdType>
void VoxelChunk::getAdjacentVoxelIDsInternal(const VoxelInt3 &voxel, const Buffer3D<VoxelIdType> &voxelIDs,
	VoxelIdType defaultID, VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID,
	VoxelIdType *outWestID)
{
	auto getIdOrDefault = [this, &voxelIDs, defaultID](const VoxelInt3 &voxel)
	{
		return this->isValidVoxel(voxel.x, voxel.y, voxel.z) ? voxelIDs.get(voxel.x, voxel.y, voxel.z) : defaultID;
	};

	const VoxelInt3 northVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::North);
	const VoxelInt3 eastVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::East);
	const VoxelInt3 southVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::South);
	const VoxelInt3 westVoxel = VoxelUtils::getAdjacentVoxelXZ(voxel, VoxelUtils::West);
	*outNorthID = getIdOrDefault(northVoxel);
	*outEastID = getIdOrDefault(eastVoxel);
	*outSouthID = getIdOrDefault(southVoxel);
	*outWestID = getIdOrDefault(westVoxel);
}

void VoxelChunk::getAdjacentVoxelMeshDefIDs(const VoxelInt3 &voxel, VoxelMeshDefID *outNorthID, VoxelMeshDefID *outEastID,
	VoxelMeshDefID *outSouthID, VoxelMeshDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelMeshDefIDs, VoxelChunk::AIR_VOXEL_MESH_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentVoxelTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID, VoxelTextureDefID *outEastID,
	VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelTextureDefIDs, VoxelChunk::AIR_VOXEL_TEXTURE_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentVoxelTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID, VoxelTraitsDefID *outEastID,
	VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelTraitsDefIDs, VoxelChunk::AIR_VOXEL_TRAITS_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::setVoxelDirty(SNInt x, int y, WEInt z)
{
	if (!this->dirtyVoxels.get(x, y, z))
	{
		this->dirtyVoxels.set(x, y, z, true);
		this->dirtyVoxelPositions.emplace_back(VoxelInt3(x, y, z));
	}
}

void VoxelChunk::setVoxelMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id)
{
	this->voxelMeshDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

void VoxelChunk::setVoxelTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id)
{
	this->voxelTextureDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

void VoxelChunk::setVoxelTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id)
{
	this->voxelTraitsDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

VoxelChunk::VoxelMeshDefID VoxelChunk::addVoxelMeshDef(VoxelMeshDefinition &&voxelMeshDef)
{
	const VoxelMeshDefID id = static_cast<VoxelMeshDefID>(this->voxelMeshDefs.size());
	this->voxelMeshDefs.emplace_back(std::move(voxelMeshDef));
	return id;
}

VoxelChunk::VoxelTextureDefID VoxelChunk::addVoxelTextureDef(VoxelTextureDefinition &&voxelTextureDef)
{
	const VoxelTextureDefID id = static_cast<VoxelTextureDefID>(this->voxelTextureDefs.size());
	this->voxelTextureDefs.emplace_back(std::move(voxelTextureDef));
	return id;
}

VoxelChunk::VoxelTraitsDefID VoxelChunk::addVoxelTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef)
{
	const VoxelTraitsDefID id = static_cast<VoxelTraitsDefID>(this->voxelTraitsDefs.size());
	this->voxelTraitsDefs.emplace_back(std::move(voxelTraitsDef));
	return id;
}

void VoxelChunk::addVoxelInst(VoxelInstance &&voxelInst)
{
	this->voxelInsts.emplace_back(std::move(voxelInst));
}

VoxelChunk::TransitionDefID VoxelChunk::addTransition(TransitionDefinition &&transition)
{
	const TransitionDefID id = static_cast<int>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(transition));
	return id;
}

VoxelChunk::TriggerDefID VoxelChunk::addTrigger(TriggerDefinition &&trigger)
{
	const TriggerDefID id = static_cast<int>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(trigger));
	return id;
}

VoxelChunk::LockDefID VoxelChunk::addLock(LockDefinition &&lock)
{
	const LockDefID id = static_cast<int>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(lock));
	return id;
}

VoxelChunk::BuildingNameID VoxelChunk::addBuildingName(std::string &&buildingName)
{
	const BuildingNameID id = static_cast<int>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(buildingName));
	return id;
}

VoxelChunk::DoorDefID VoxelChunk::addDoorDef(DoorDefinition &&door)
{
	const DoorDefID id = static_cast<int>(this->doorDefs.size());
	this->doorDefs.emplace_back(std::move(door));
	return id;
}

VoxelChunk::ChasmDefID VoxelChunk::addChasmDef(ChasmDefinition &&chasm)
{
	const ChasmDefID id = static_cast<int>(this->chasmDefs.size());
	this->chasmDefs.emplace_back(std::move(chasm));
	return id;
}

void VoxelChunk::addTransitionPosition(VoxelChunk::TransitionDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->transitionDefIndices.find(voxel) == this->transitionDefIndices.end());
	this->transitionDefIndices.emplace(voxel, id);
}

void VoxelChunk::addTriggerPosition(VoxelChunk::TriggerDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->triggerDefIndices.find(voxel) == this->triggerDefIndices.end());
	this->triggerDefIndices.emplace(voxel, id);
}

void VoxelChunk::addLockPosition(VoxelChunk::LockDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->lockDefIndices.find(voxel) == this->lockDefIndices.end());
	this->lockDefIndices.emplace(voxel, id);
}

void VoxelChunk::addBuildingNamePosition(VoxelChunk::BuildingNameID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->buildingNameIndices.find(voxel) == this->buildingNameIndices.end());
	this->buildingNameIndices.emplace(voxel, id);
}

void VoxelChunk::addDoorPosition(DoorDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->doorDefIndices.find(voxel) == this->doorDefIndices.end());
	this->doorDefIndices.emplace(voxel, id);
}

void VoxelChunk::addChasmPosition(ChasmDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->chasmDefIndices.find(voxel) == this->chasmDefIndices.end());
	this->chasmDefIndices.emplace(voxel, id);
}

void VoxelChunk::removeVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
{
	for (int i = 0; i < static_cast<int>(this->voxelInsts.size()); i++)
	{
		const VoxelInstance &voxelInst = this->voxelInsts[i];
		if ((voxelInst.getX() == voxel.x) && (voxelInst.getY() == voxel.y) &&
			(voxelInst.getZ() == voxel.z) && (voxelInst.getType() == type))
		{
			this->voxelInsts.erase(this->voxelInsts.begin() + i);
			break;
		}
	}
}

void VoxelChunk::clear()
{
	this->voxelMeshDefs.clear();
	this->voxelTextureDefs.clear();
	this->voxelTraitsDefs.clear();
	this->voxelMeshDefIDs.clear();
	this->voxelTextureDefIDs.clear();
	this->voxelTraitsDefIDs.clear();
	this->dirtyVoxels.clear();
	this->dirtyVoxelPositions.clear();
	this->voxelInsts.clear();
	this->transitionDefs.clear();
	this->triggerDefs.clear();
	this->lockDefs.clear();
	this->buildingNames.clear();
	this->doorDefs.clear();
	this->chasmDefs.clear();
	this->transitionDefIndices.clear();
	this->triggerDefIndices.clear();
	this->lockDefIndices.clear();
	this->buildingNameIndices.clear();
	this->doorDefIndices.clear();
	this->chasmDefIndices.clear();
	this->position = ChunkInt2();
}

void VoxelChunk::clearDirtyVoxels()
{
	this->dirtyVoxels.fill(false);
	this->dirtyVoxelPositions.clear();
}

void VoxelChunk::handleVoxelInstState(VoxelInstance &voxelInst, const CoordDouble3 &playerCoord,
	double ceilingScale, AudioManager &audioManager)
{
	if (voxelInst.getType() == VoxelInstance::Type::OpenDoor)
	{
		VoxelInstance::DoorState &doorState = voxelInst.getDoorState();
		if (doorState.getStateType() != VoxelInstance::DoorState::StateType::Closing)
		{
			// If the player is far enough away, set the door to closing and play the on-closing sound at the center of
			// the voxel if it is defined for the door.
			const VoxelInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
			const CoordDouble3 voxelCoord(this->position, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
			const VoxelDouble3 diff = playerCoord - voxelCoord;

			constexpr double closeDistSqr = ArenaLevelUtils::DOOR_CLOSE_DISTANCE * ArenaLevelUtils::DOOR_CLOSE_DISTANCE;
			const double distSqr = diff.lengthSquared();

			if (distSqr >= closeDistSqr)
			{
				doorState.setStateType(VoxelInstance::DoorState::StateType::Closing);

				// Play closing sound if it is defined for the door.
				VoxelChunk::DoorDefID doorDefID;
				if (!this->tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
				{
					DebugCrash("Expected door def ID to exist.");
				}
				
				const DoorDefinition &doorDef = this->getDoorDef(doorDefID);
				const DoorDefinition::CloseSoundDef &closeSoundDef = doorDef.getCloseSound();
				if (closeSoundDef.closeType == DoorDefinition::CloseType::OnClosing)
				{
					const NewDouble3 absoluteSoundPosition = VoxelUtils::coordToNewPoint(voxelCoord);
					audioManager.playSound(closeSoundDef.soundFilename, absoluteSoundPosition);
				}
			}
		}
	}
}

void VoxelChunk::handleVoxelInstFinished(VoxelInstance &voxelInst, double ceilingScale, AudioManager &audioManager)
{
	const VoxelInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());

	if (voxelInst.getType() == VoxelInstance::Type::OpenDoor)
	{
		// Play closed sound if it is defined for the door.
		VoxelChunk::DoorDefID doorDefID;
		if (!this->tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
		{
			DebugCrash("Expected door def ID to exist.");
		}

		const DoorDefinition &doorDef = this->getDoorDef(doorDefID);
		const DoorDefinition::CloseSoundDef &closeSoundDef = doorDef.getCloseSound();
		if (closeSoundDef.closeType == DoorDefinition::CloseType::OnClosed)
		{
			const CoordDouble3 soundCoord(this->position, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
			const NewDouble3 absoluteSoundPosition = VoxelUtils::coordToNewPoint(soundCoord);
			audioManager.playSound(closeSoundDef.soundFilename, absoluteSoundPosition);
		}
	}
	else if (voxelInst.getType() == VoxelInstance::Type::Fading)
	{
		// Convert the faded voxel to air or a chasm depending on the Y coordinate.
		if (voxel.y == 0)
		{
			// Replace floor voxel with chasm.
			DebugNotImplementedMsg("Floor voxel replacement.");
			/*const std::optional<VoxelID> replacementVoxelID = [this]() -> std::optional<VoxelID>
			{
				// Try to get from existing voxel defs.
				for (int i = 0; i < static_cast<int>(this->voxelDefs.size()); i++)
				{
					if (this->activeVoxelDefs[i])
					{
						const VoxelDefinition &voxelDef = this->voxelDefs[i];
						if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
						{
							const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;
							if (chasmData.type == ArenaTypes::ChasmType::Wet)
							{
								return static_cast<VoxelID>(i);
							}
						}
					}
				}

				// No existing water chasm voxel definition. Make a new one?
				// @todo: This could be handled better, since walls are just one choice for the texture.
				// - maybe need a 'fallbackWaterChasm' texture asset ref in LevelInfoDefinition.
				const TextureAsset *replacementTextureAsset = [this]() -> const TextureAsset*
				{
					for (int i = 0; i < static_cast<int>(this->voxelDefs.size()); i++)
					{
						if (this->activeVoxelDefs[i])
						{
							const VoxelDefinition &voxelDef = this->voxelDefs[i];
							if (voxelDef.type == ArenaTypes::VoxelType::Wall)
							{
								const VoxelDefinition::WallData &wallData = voxelDef.wall;
								return &wallData.sideTextureAsset;
							}
						}
					}

					return nullptr;
				}();

				DebugAssert(replacementTextureAsset != nullptr);
				VoxelDefinition voxelDef = VoxelDefinition::makeChasm(
					TextureAsset(*replacementTextureAsset), ArenaTypes::ChasmType::Wet);

				VoxelID voxelID;
				if (this->tryAddVoxelDef(std::move(voxelDef), &voxelID))
				{
					return voxelID;
				}
				else
				{
					return std::nullopt;
				}
			}();

			DebugAssertMsg(replacementVoxelID.has_value(), "Couldn't find replacement for faded chasm voxel.");
			this->setVoxelID(voxel.x, voxel.y, voxel.z, *replacementVoxelID);
			DebugNotImplementedMsg("Need to implement for voxel mesh def.");*/
		}
		else
		{
			// Air voxel.
			this->setVoxelMeshDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_VOXEL_MESH_DEF_ID);
			this->setVoxelTextureDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_VOXEL_TEXTURE_DEF_ID);
			this->setVoxelTraitsDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_VOXEL_TRAITS_DEF_ID);
		}
	}
}

void VoxelChunk::handleVoxelInstPostFinished(VoxelInstance &voxelInst, std::vector<int> &voxelInstIndicesToDestroy)
{
	if (voxelInst.getType() == VoxelInstance::Type::Fading)
	{
		if (voxelInst.getY() == 0)
		{
			// Need to handle the voxel instance for the new chasm in this voxel, and update any adjacent
			// chasms too.
			DebugNotImplementedMsg("handleVoxelInstPostFinished() chasm voxel.");
			/*auto tryUpdateAdjacentVoxel = [this, &voxelInst, &voxelInstIndicesToDestroy](const VoxelInt2 &direction)
			{
				const VoxelInt3 voxel(
					voxelInst.getX() + direction.x,
					voxelInst.getY(),
					voxelInst.getZ() + direction.y);
				const bool isValidVoxel = (voxel.x >= 0) && (voxel.x < VoxelChunk::WIDTH) && (voxel.y >= 0) &&
					(voxel.y < this->getHeight()) && (voxel.z >= 0) && (voxel.z < VoxelChunk::DEPTH);
				
				if (isValidVoxel)
				{
					const VoxelChunk::VoxelID voxelID = this->getVoxelID(voxel.x, voxel.y, voxel.z);
					const VoxelDefinition &voxelDef = this->getVoxelDef(voxelID);
					if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
					{
						const VoxelDefinition *northDef, *eastDef, *southDef, *westDef;
						this->getAdjacentVoxelDefs(voxel, &northDef, &eastDef, &southDef, &westDef);

						const bool hasNorthFace = (northDef != nullptr) && northDef->allowsChasmFace();
						const bool hasEastFace = (eastDef != nullptr) && eastDef->allowsChasmFace();
						const bool hasSouthFace = (southDef != nullptr) && southDef->allowsChasmFace();
						const bool hasWestFace = (westDef != nullptr) && westDef->allowsChasmFace();
						const bool hasAnyFaces = hasNorthFace || hasEastFace || hasSouthFace || hasWestFace;

						constexpr VoxelInstance::Type voxelInstType = VoxelInstance::Type::Chasm;
						const std::optional<int> existingVoxelInstIndex = this->tryGetVoxelInstIndex(voxel, voxelInstType);
						if (existingVoxelInstIndex.has_value())
						{
							if (hasAnyFaces)
							{
								// Update existing voxel instance.
								VoxelInstance &existingVoxelInst = this->voxelInsts[*existingVoxelInstIndex];
								VoxelInstance::ChasmState &chasmState = existingVoxelInst.getChasmState();
								chasmState.init(hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
							}
							else
							{
								// Delete unneeded voxel instance (duplicate indices to destroy are handled later).
								voxelInstIndicesToDestroy.push_back(*existingVoxelInstIndex);
							}
						}
						else
						{
							if (hasAnyFaces)
							{
								// Add new voxel instance for the adjacent chasm.
								VoxelInstance newVoxelInst = VoxelInstance::makeChasm(voxel.x, voxel.y, voxel.z,
									hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
								this->addVoxelInst(std::move(newVoxelInst));
							}
						}
					}
				}
			};

			// This voxel's definition has been changed to a chasm. Need to add a new voxel instance for the chasm
			// if there are chasm walls, AND update adjacent voxels if they are chasms too. This needs to be done
			// after all fading voxels that finish this frame have finished because they might be adjacent to each other.
			const VoxelInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
			const VoxelDefinition *northDef, *eastDef, *southDef, *westDef;
			this->getAdjacentVoxelDefs(voxel, &northDef, &eastDef, &southDef, &westDef);

			const bool hasNorthFace = (northDef != nullptr) && northDef->allowsChasmFace();
			const bool hasEastFace = (eastDef != nullptr) && eastDef->allowsChasmFace();
			const bool hasSouthFace = (southDef != nullptr) && southDef->allowsChasmFace();
			const bool hasWestFace = (westDef != nullptr) && westDef->allowsChasmFace();
			if (hasNorthFace || hasEastFace || hasSouthFace || hasWestFace)
			{
				VoxelInstance chasmVoxelInst = VoxelInstance::makeChasm(voxel.x, voxel.y, voxel.z,
					hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
				this->addVoxelInst(std::move(chasmVoxelInst));
			}

			tryUpdateAdjacentVoxel(VoxelUtils::North);
			tryUpdateAdjacentVoxel(VoxelUtils::East);
			tryUpdateAdjacentVoxel(VoxelUtils::South);
			tryUpdateAdjacentVoxel(VoxelUtils::West);*/
		}
	}
}

void VoxelChunk::update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager)
{
	// Need to track voxel instances that finished fading because certain ones are converted to
	// context-sensitive voxels on completion.
	std::vector<VoxelInstance*> voxelInstsToPostFinish;
	std::vector<int> voxelInstIndicesToDestroy;

	for (int i = static_cast<int>(this->voxelInsts.size()) - 1; i >= 0; i--)
	{
		VoxelInstance &voxelInst = this->voxelInsts[i];
		voxelInst.update(dt);

		// See if the voxel instance is in a state that needs more behavior to be run, or if it can be
		// removed because it no longer has interesting state.
		if (voxelInst.hasRelevantState())
		{
			this->handleVoxelInstState(voxelInst, playerCoord, ceilingScale, audioManager);
		}
		else
		{
			// Do the voxel instance's "on destroy" action, if any.
			this->handleVoxelInstFinished(voxelInst, ceilingScale, audioManager);

			// Certain voxel instances need another step of shutdown since they need to run after all
			// other voxel instances have been updated this frame, due to being context-sensitive.
			const bool needsPostShutdown = (voxelInst.getType() == VoxelInstance::Type::Fading) && (voxelInst.getY() == 0);
			if (needsPostShutdown)
			{
				voxelInstsToPostFinish.push_back(&voxelInst);
			}

			voxelInstIndicesToDestroy.push_back(i);
		}

		this->setVoxelDirty(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
	}

	for (VoxelInstance *voxelInst : voxelInstsToPostFinish)
	{
		this->handleVoxelInstPostFinished(*voxelInst, voxelInstIndicesToDestroy);
	}

	// Due to the extra complexity of adjacent voxel instances potentially being destroyed during post-finish,
	// need to do some sanitization here on the indices to destroy.
	std::sort(voxelInstIndicesToDestroy.begin(), voxelInstIndicesToDestroy.end());
	const auto uniqueIter = std::unique(voxelInstIndicesToDestroy.begin(), voxelInstIndicesToDestroy.end());
	voxelInstIndicesToDestroy.erase(uniqueIter, voxelInstIndicesToDestroy.end());

	for (int i = static_cast<int>(voxelInstIndicesToDestroy.size()) - 1; i >= 0; i--)
	{
		const int index = voxelInstIndicesToDestroy[i];
		this->voxelInsts.erase(this->voxelInsts.begin() + index);
	}
}
