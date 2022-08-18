#include <algorithm>

#include "Chunk.h"
#include "../Audio/AudioManager.h"

#include "components/debug/Debug.h"

void Chunk::init(const ChunkInt2 &position, int height)
{
	// Set all voxels to air.
	this->voxelMeshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxelMeshDefIDs.fill(Chunk::AIR_VOXEL_MESH_DEF_ID);

	this->voxelTextureDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxelTextureDefIDs.fill(Chunk::AIR_VOXEL_TEXTURE_DEF_ID);

	this->voxelTraitsDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxelTraitsDefIDs.fill(Chunk::AIR_VOXEL_TRAITS_DEF_ID);

	// Let the first voxel definition (air) be usable immediately. All default voxel IDs can safely point to it.
	this->voxelMeshDefs.emplace_back(VoxelMeshDefinition());
	this->voxelTextureDefs.emplace_back(VoxelTextureDefinition());
	this->voxelTraitsDefs.emplace_back(VoxelTraitsDefinition());

	this->position = position;
}

const ChunkInt2 &Chunk::getPosition() const
{
	return this->position;
}

bool Chunk::isValidVoxel(SNInt x, int y, WEInt z) const
{
	return (x >= 0) && (x < Chunk::WIDTH) && (y >= 0) && (y < this->getHeight()) && (z >= 0) && (z < Chunk::DEPTH);
}

int Chunk::getHeight() const
{
	DebugAssert(this->voxelMeshDefIDs.getHeight() == this->voxelTextureDefIDs.getHeight());
	DebugAssert(this->voxelMeshDefIDs.getHeight() == this->voxelTraitsDefIDs.getHeight());
	return this->voxelMeshDefIDs.getHeight();
}

Chunk::VoxelMeshDefID Chunk::getVoxelMeshDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelMeshDefIDs.get(x, y, z);
}

Chunk::VoxelTextureDefID Chunk::getVoxelTextureDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelTextureDefIDs.get(x, y, z);
}

Chunk::VoxelTraitsDefID Chunk::getVoxelTraitsDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelTraitsDefIDs.get(x, y, z);
}

int Chunk::getVoxelMeshDefCount() const
{
	return static_cast<int>(this->voxelMeshDefs.size());
}

int Chunk::getVoxelTextureDefCount() const
{
	return static_cast<int>(this->voxelTextureDefs.size());
}

int Chunk::getVoxelTraitsDefCount() const
{
	return static_cast<int>(this->voxelTraitsDefs.size());
}

const VoxelMeshDefinition &Chunk::getVoxelMeshDef(int index) const
{
	DebugAssertIndex(this->voxelMeshDefs, index);
	return this->voxelMeshDefs[index];
}

const VoxelTextureDefinition &Chunk::getVoxelTextureDef(int index) const
{
	DebugAssertIndex(this->voxelTextureDefs, index);
	return this->voxelTextureDefs[index];
}

const VoxelTraitsDefinition &Chunk::getVoxelTraitsDef(int index) const
{
	DebugAssertIndex(this->voxelTraitsDefs, index);
	return this->voxelTraitsDefs[index];
}

int Chunk::getDirtyVoxelCount() const
{
	return static_cast<int>(this->dirtyVoxels.size());
}

const VoxelInt3 &Chunk::getDirtyVoxel(int index) const
{
	DebugAssertIndex(this->dirtyVoxels, index);
	return this->dirtyVoxels[index];
}

int Chunk::getVoxelInstCount() const
{
	return static_cast<int>(this->voxelInsts.size());
}

VoxelInstance &Chunk::getVoxelInst(int index)
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

const VoxelInstance &Chunk::getVoxelInst(int index) const
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

std::optional<int> Chunk::tryGetVoxelInstIndex(const VoxelInt3 &voxel, VoxelInstance::Type type) const
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

VoxelInstance *Chunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
{
	const std::optional<int> index = this->tryGetVoxelInstIndex(voxel, type);
	return index.has_value() ? &this->voxelInsts[*index] : nullptr;
}

const VoxelInstance *Chunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type) const
{
	const std::optional<int> index = this->tryGetVoxelInstIndex(voxel, type);
	return index.has_value() ? &this->voxelInsts[*index] : nullptr;
}

const TransitionDefinition *Chunk::tryGetTransition(const VoxelInt3 &voxel) const
{
	const auto iter = this->transitionDefIndices.find(voxel);
	if (iter != this->transitionDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->transitionDefs, index);
		return &this->transitionDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const TriggerDefinition *Chunk::tryGetTrigger(const VoxelInt3 &voxel) const
{
	const auto iter = this->triggerDefIndices.find(voxel);
	if (iter != this->triggerDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->triggerDefs, index);
		return &this->triggerDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const LockDefinition *Chunk::tryGetLock(const VoxelInt3 &voxel) const
{
	const auto iter = this->lockDefIndices.find(voxel);
	if (iter != this->lockDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->lockDefs, index);
		return &this->lockDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const std::string *Chunk::tryGetBuildingName(const VoxelInt3 &voxel) const
{
	const auto iter = this->buildingNameIndices.find(voxel);
	if (iter != this->buildingNameIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->buildingNames, index);
		return &this->buildingNames[index];
	}
	else
	{
		return nullptr;
	}
}

const DoorDefinition *Chunk::tryGetDoor(const VoxelInt3 &voxel) const
{
	const auto iter = this->doorDefIndices.find(voxel);
	if (iter != this->doorDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->doorDefs, index);
		return &this->doorDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const ChasmDefinition &Chunk::getChasm(ChasmID id) const
{
	DebugAssertIndex(this->chasmDefs, id);
	return this->chasmDefs[id];
}

const ChasmDefinition *Chunk::tryGetChasm(const VoxelInt3 &voxel) const
{
	const auto iter = this->chasmDefIndices.find(voxel);
	if (iter != this->chasmDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->chasmDefs, index);
		return &this->chasmDefs[index];
	}
	else
	{
		return nullptr;
	}
}

template <typename VoxelIdType>
void Chunk::getAdjacentVoxelIDsInternal(const VoxelInt3 &voxel, const Buffer3D<VoxelIdType> &voxelIDs,
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

void Chunk::getAdjacentVoxelMeshDefIDs(const VoxelInt3 &voxel, VoxelMeshDefID *outNorthID, VoxelMeshDefID *outEastID,
	VoxelMeshDefID *outSouthID, VoxelMeshDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelMeshDefIDs, Chunk::AIR_VOXEL_MESH_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void Chunk::getAdjacentVoxelTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID, VoxelTextureDefID *outEastID,
	VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelTextureDefIDs, Chunk::AIR_VOXEL_TEXTURE_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void Chunk::getAdjacentVoxelTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID, VoxelTraitsDefID *outEastID,
	VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID)
{
	this->getAdjacentVoxelIDsInternal(voxel, this->voxelTraitsDefIDs, Chunk::AIR_VOXEL_TRAITS_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void Chunk::setVoxelDirty(SNInt x, int y, WEInt z)
{
	const VoxelInt3 pos(x, y, z);
	const auto dirtyIter = std::find(this->dirtyVoxels.begin(), this->dirtyVoxels.end(), pos);
	if (dirtyIter == this->dirtyVoxels.end())
	{
		this->dirtyVoxels.emplace_back(pos);
	}
}

void Chunk::setVoxelMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id)
{
	this->voxelMeshDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

void Chunk::setVoxelTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id)
{
	this->voxelTextureDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

void Chunk::setVoxelTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id)
{
	this->voxelTraitsDefIDs.set(x, y, z, id);
	this->setVoxelDirty(x, y, z);
}

Chunk::VoxelMeshDefID Chunk::addVoxelMeshDef(VoxelMeshDefinition &&voxelMeshDef)
{
	const VoxelMeshDefID id = static_cast<VoxelMeshDefID>(this->voxelMeshDefs.size());
	this->voxelMeshDefs.emplace_back(std::move(voxelMeshDef));
	return id;
}

Chunk::VoxelTextureDefID Chunk::addVoxelTextureDef(VoxelTextureDefinition &&voxelTextureDef)
{
	const VoxelTextureDefID id = static_cast<VoxelTextureDefID>(this->voxelTextureDefs.size());
	this->voxelTextureDefs.emplace_back(std::move(voxelTextureDef));
	return id;
}

Chunk::VoxelTraitsDefID Chunk::addVoxelTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef)
{
	const VoxelTraitsDefID id = static_cast<VoxelTraitsDefID>(this->voxelTraitsDefs.size());
	this->voxelTraitsDefs.emplace_back(std::move(voxelTraitsDef));
	return id;
}

void Chunk::addVoxelInst(VoxelInstance &&voxelInst)
{
	this->voxelInsts.emplace_back(std::move(voxelInst));
}

Chunk::TransitionID Chunk::addTransition(TransitionDefinition &&transition)
{
	const TransitionID id = static_cast<int>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(transition));
	return id;
}

Chunk::TriggerID Chunk::addTrigger(TriggerDefinition &&trigger)
{
	const TriggerID id = static_cast<int>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(trigger));
	return id;
}

Chunk::LockID Chunk::addLock(LockDefinition &&lock)
{
	const LockID id = static_cast<int>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(lock));
	return id;
}

Chunk::BuildingNameID Chunk::addBuildingName(std::string &&buildingName)
{
	const BuildingNameID id = static_cast<int>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(buildingName));
	return id;
}

Chunk::DoorID Chunk::addDoorDef(DoorDefinition &&door)
{
	const DoorID id = static_cast<int>(this->doorDefs.size());
	this->doorDefs.emplace_back(std::move(door));
	return id;
}

Chunk::ChasmID Chunk::addChasmDef(ChasmDefinition &&chasm)
{
	const ChasmID id = static_cast<int>(this->chasmDefs.size());
	this->chasmDefs.emplace_back(std::move(chasm));
	return id;
}

void Chunk::addTransitionPosition(Chunk::TransitionID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->transitionDefIndices.find(voxel) == this->transitionDefIndices.end());
	this->transitionDefIndices.emplace(voxel, id);
}

void Chunk::addTriggerPosition(Chunk::TriggerID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->triggerDefIndices.find(voxel) == this->triggerDefIndices.end());
	this->triggerDefIndices.emplace(voxel, id);
}

void Chunk::addLockPosition(Chunk::LockID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->lockDefIndices.find(voxel) == this->lockDefIndices.end());
	this->lockDefIndices.emplace(voxel, id);
}

void Chunk::addBuildingNamePosition(Chunk::BuildingNameID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->buildingNameIndices.find(voxel) == this->buildingNameIndices.end());
	this->buildingNameIndices.emplace(voxel, id);
}

void Chunk::addDoorPosition(DoorID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->doorDefIndices.find(voxel) == this->doorDefIndices.end());
	this->doorDefIndices.emplace(voxel, id);
}

void Chunk::addChasmPosition(ChasmID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->chasmDefIndices.find(voxel) == this->chasmDefIndices.end());
	this->chasmDefIndices.emplace(voxel, id);
}

void Chunk::removeVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
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

void Chunk::clear()
{
	this->voxelMeshDefs.clear();
	this->voxelTextureDefs.clear();
	this->voxelTraitsDefs.clear();
	this->voxelMeshDefIDs.clear();
	this->voxelTextureDefIDs.clear();
	this->voxelTraitsDefIDs.clear();
	this->dirtyVoxels.clear();
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

void Chunk::clearDirtyVoxels()
{
	this->dirtyVoxels.clear();
}

void Chunk::handleVoxelInstState(VoxelInstance &voxelInst, const CoordDouble3 &playerCoord,
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
				const DoorDefinition *doorDef = this->tryGetDoor(voxel);
				DebugAssert(doorDef != nullptr);
				const DoorDefinition::CloseSoundDef &closeSoundDef = doorDef->getCloseSound();
				if (closeSoundDef.closeType == DoorDefinition::CloseType::OnClosing)
				{
					const NewDouble3 absoluteSoundPosition = VoxelUtils::coordToNewPoint(voxelCoord);
					audioManager.playSound(closeSoundDef.soundFilename, absoluteSoundPosition);
				}
			}
		}
	}
}

void Chunk::handleVoxelInstFinished(VoxelInstance &voxelInst, double ceilingScale, AudioManager &audioManager)
{
	const VoxelInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());

	if (voxelInst.getType() == VoxelInstance::Type::OpenDoor)
	{
		// Play closed sound if it is defined for the door.
		const DoorDefinition *doorDef = this->tryGetDoor(voxel);
		DebugAssert(doorDef != nullptr);
		const DoorDefinition::CloseSoundDef &closeSoundDef = doorDef->getCloseSound();
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
			this->setVoxelMeshDefID(voxel.x, voxel.y, voxel.z, Chunk::AIR_VOXEL_MESH_DEF_ID);
			this->setVoxelTextureDefID(voxel.x, voxel.y, voxel.z, Chunk::AIR_VOXEL_TEXTURE_DEF_ID);
			this->setVoxelTraitsDefID(voxel.x, voxel.y, voxel.z, Chunk::AIR_VOXEL_TRAITS_DEF_ID);
		}
	}
}

void Chunk::handleVoxelInstPostFinished(VoxelInstance &voxelInst, std::vector<int> &voxelInstIndicesToDestroy)
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
				const bool isValidVoxel = (voxel.x >= 0) && (voxel.x < Chunk::WIDTH) && (voxel.y >= 0) &&
					(voxel.y < this->getHeight()) && (voxel.z >= 0) && (voxel.z < Chunk::DEPTH);
				
				if (isValidVoxel)
				{
					const Chunk::VoxelID voxelID = this->getVoxelID(voxel.x, voxel.y, voxel.z);
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

void Chunk::update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager)
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
