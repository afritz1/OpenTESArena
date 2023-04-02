#include <algorithm>
#include <type_traits>

#include "VoxelChunk.h"
#include "../Audio/AudioManager.h"

#include "components/debug/Debug.h"

void VoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first voxel definition (air) be usable immediately. All default voxel IDs can safely point to it.
	this->meshDefs.emplace_back(VoxelMeshDefinition());
	this->textureDefs.emplace_back(VoxelTextureDefinition());
	this->traitsDefs.emplace_back(VoxelTraitsDefinition());

	// Set all voxels to air.
	this->meshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->meshDefIDs.fill(VoxelChunk::AIR_MESH_DEF_ID);

	this->textureDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->textureDefIDs.fill(VoxelChunk::AIR_TEXTURE_DEF_ID);

	this->traitsDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->traitsDefIDs.fill(VoxelChunk::AIR_TRAITS_DEF_ID);

	this->dirtyVoxelTypes.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->dirtyVoxelTypes.fill(static_cast<VoxelDirtyType>(0));
	this->dirtyMeshDefPositions.reserve(Chunk::WIDTH * height * Chunk::DEPTH);
}

void VoxelChunk::getAdjacentMeshDefIDs(const VoxelInt3 &voxel, VoxelMeshDefID *outNorthID, VoxelMeshDefID *outEastID,
	VoxelMeshDefID *outSouthID, VoxelMeshDefID *outWestID)
{
	this->getAdjacentIDsInternal(voxel, this->meshDefIDs, VoxelChunk::AIR_MESH_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID, VoxelTextureDefID *outEastID,
	VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID)
{
	this->getAdjacentIDsInternal(voxel, this->textureDefIDs, VoxelChunk::AIR_TEXTURE_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

void VoxelChunk::getAdjacentTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID, VoxelTraitsDefID *outEastID,
	VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID)
{
	this->getAdjacentIDsInternal(voxel, this->traitsDefIDs, VoxelChunk::AIR_TRAITS_DEF_ID, outNorthID, outEastID, outSouthID, outWestID);
}

int VoxelChunk::getMeshDefCount() const
{
	return static_cast<int>(this->meshDefs.size());
}

int VoxelChunk::getTextureDefCount() const
{
	return static_cast<int>(this->textureDefs.size());
}

int VoxelChunk::getTraitsDefCount() const
{
	return static_cast<int>(this->traitsDefs.size());
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

const VoxelMeshDefinition &VoxelChunk::getMeshDef(VoxelMeshDefID id) const
{
	DebugAssertIndex(this->meshDefs, id);
	return this->meshDefs[id];
}

const VoxelTextureDefinition &VoxelChunk::getTextureDef(VoxelTextureDefID id) const
{
	DebugAssertIndex(this->textureDefs, id);
	return this->textureDefs[id];
}

const VoxelTraitsDefinition &VoxelChunk::getTraitsDef(VoxelTraitsDefID id) const
{
	DebugAssertIndex(this->traitsDefs, id);
	return this->traitsDefs[id];
}

const TransitionDefinition &VoxelChunk::getTransitionDef(TransitionDefID id) const
{
	DebugAssertIndex(this->transitionDefs, id);
	return this->transitionDefs[id];
}

const VoxelTriggerDefinition &VoxelChunk::getTriggerDef(TriggerDefID id) const
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

VoxelChunk::VoxelMeshDefID VoxelChunk::getMeshDefID(SNInt x, int y, WEInt z) const
{
	return this->meshDefIDs.get(x, y, z);
}

VoxelChunk::VoxelTextureDefID VoxelChunk::getTextureDefID(SNInt x, int y, WEInt z) const
{
	return this->textureDefIDs.get(x, y, z);
}

VoxelChunk::VoxelTraitsDefID VoxelChunk::getTraitsDefID(SNInt x, int y, WEInt z) const
{
	return this->traitsDefIDs.get(x, y, z);
}

int VoxelChunk::getDirtyMeshDefPositionCount() const
{
	return static_cast<int>(this->dirtyMeshDefPositions.size());
}

int VoxelChunk::getDirtyDoorAnimInstPositionCount() const
{
	return static_cast<int>(this->dirtyDoorAnimInstPositions.size());
}

int VoxelChunk::getDirtyDoorVisInstPositionCount() const
{
	return static_cast<int>(this->dirtyDoorVisInstPositions.size());
}

int VoxelChunk::getDirtyFadeAnimInstPositionCount() const
{
	return static_cast<int>(this->dirtyFadeAnimInstPositions.size());
}

int VoxelChunk::getDirtyChasmWallInstPositionCount() const
{
	return static_cast<int>(this->dirtyChasmWallInstPositions.size());
}

const VoxelInt3 &VoxelChunk::getDirtyMeshDefPosition(int index) const
{
	DebugAssertIndex(this->dirtyMeshDefPositions, index);
	return this->dirtyMeshDefPositions[index];
}

const VoxelInt3 &VoxelChunk::getDirtyDoorAnimInstPosition(int index) const
{
	DebugAssertIndex(this->dirtyDoorAnimInstPositions, index);
	return this->dirtyDoorAnimInstPositions[index];
}

const VoxelInt3 &VoxelChunk::getDirtyDoorVisInstPosition(int index) const
{
	DebugAssertIndex(this->dirtyDoorVisInstPositions, index);
	return this->dirtyDoorVisInstPositions[index];
}

const VoxelInt3 &VoxelChunk::getDirtyFadeAnimInstPosition(int index) const
{
	DebugAssertIndex(this->dirtyFadeAnimInstPositions, index);
	return this->dirtyFadeAnimInstPositions[index];
}

const VoxelInt3 &VoxelChunk::getDirtyChasmWallInstPosition(int index) const
{
	DebugAssertIndex(this->dirtyChasmWallInstPositions, index);
	return this->dirtyChasmWallInstPositions[index];
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

int VoxelChunk::getDoorAnimInstCount() const
{
	return static_cast<int>(this->doorAnimInsts.size());
}

int VoxelChunk::getFadeAnimInstCount() const
{
	return static_cast<int>(this->fadeAnimInsts.size());
}

const VoxelDoorAnimationInstance &VoxelChunk::getDoorAnimInst(int index) const
{
	DebugAssertIndex(this->doorAnimInsts, index);
	return this->doorAnimInsts[index];
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

const VoxelFadeAnimationInstance &VoxelChunk::getFadeAnimInst(int index) const
{
	DebugAssertIndex(this->fadeAnimInsts, index);
	return this->fadeAnimInsts[index];
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

int VoxelChunk::getChasmWallInstCount() const
{
	return static_cast<int>(this->chasmWallInsts.size());
}

int VoxelChunk::getDoorVisibilityInstCount() const
{
	return static_cast<int>(this->doorVisInsts.size());
}

int VoxelChunk::getTriggerInstCount() const
{
	return static_cast<int>(this->triggerInsts.size());
}

VoxelChasmWallInstance &VoxelChunk::getChasmWallInst(int index)
{
	DebugAssertIndex(this->chasmWallInsts, index);
	return this->chasmWallInsts[index];
}

const VoxelChasmWallInstance &VoxelChunk::getChasmWallInst(int index) const
{
	DebugAssertIndex(this->chasmWallInsts, index);
	return this->chasmWallInsts[index];
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

VoxelDoorVisibilityInstance &VoxelChunk::getDoorVisibilityInst(int index)
{
	DebugAssertIndex(this->doorVisInsts, index);
	return this->doorVisInsts[index];
}

const VoxelDoorVisibilityInstance &VoxelChunk::getDoorVisibilityInst(int index) const
{
	DebugAssertIndex(this->doorVisInsts, index);
	return this->doorVisInsts[index];
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

const VoxelTriggerInstance &VoxelChunk::getTriggerInst(int index) const
{
	DebugAssertIndex(this->triggerInsts, index);
	return this->triggerInsts[index];
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

void VoxelChunk::setMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id)
{
	this->meshDefIDs.set(x, y, z, id);
	this->setMeshDefDirty(x, y, z);
}

void VoxelChunk::setTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id)
{
	this->textureDefIDs.set(x, y, z, id);
}

void VoxelChunk::setTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id)
{
	this->traitsDefIDs.set(x, y, z, id);
}

VoxelChunk::VoxelMeshDefID VoxelChunk::addMeshDef(VoxelMeshDefinition &&voxelMeshDef)
{
	const VoxelMeshDefID id = static_cast<VoxelMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(voxelMeshDef));
	return id;
}

VoxelChunk::VoxelTextureDefID VoxelChunk::addTextureDef(VoxelTextureDefinition &&voxelTextureDef)
{
	const VoxelTextureDefID id = static_cast<VoxelTextureDefID>(this->textureDefs.size());
	this->textureDefs.emplace_back(std::move(voxelTextureDef));
	return id;
}

VoxelChunk::VoxelTraitsDefID VoxelChunk::addTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef)
{
	const VoxelTraitsDefID id = static_cast<VoxelTraitsDefID>(this->traitsDefs.size());
	this->traitsDefs.emplace_back(std::move(voxelTraitsDef));
	return id;
}

VoxelChunk::TransitionDefID VoxelChunk::addTransitionDef(TransitionDefinition &&transition)
{
	const TransitionDefID id = static_cast<int>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(transition));
	return id;
}

VoxelChunk::TriggerDefID VoxelChunk::addTriggerDef(VoxelTriggerDefinition &&trigger)
{
	const TriggerDefID id = static_cast<int>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(trigger));
	return id;
}

VoxelChunk::LockDefID VoxelChunk::addLockDef(LockDefinition &&lock)
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

void VoxelChunk::addTransitionDefPosition(VoxelChunk::TransitionDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->transitionDefIndices.find(voxel) == this->transitionDefIndices.end());
	this->transitionDefIndices.emplace(voxel, id);
}

void VoxelChunk::addTriggerDefPosition(VoxelChunk::TriggerDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->triggerDefIndices.find(voxel) == this->triggerDefIndices.end());
	this->triggerDefIndices.emplace(voxel, id);
}

void VoxelChunk::addLockDefPosition(VoxelChunk::LockDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->lockDefIndices.find(voxel) == this->lockDefIndices.end());
	this->lockDefIndices.emplace(voxel, id);
}

void VoxelChunk::addBuildingNamePosition(VoxelChunk::BuildingNameID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->buildingNameIndices.find(voxel) == this->buildingNameIndices.end());
	this->buildingNameIndices.emplace(voxel, id);
}

void VoxelChunk::addDoorDefPosition(DoorDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->doorDefIndices.find(voxel) == this->doorDefIndices.end());
	this->doorDefIndices.emplace(voxel, id);
}

void VoxelChunk::addChasmDefPosition(ChasmDefID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->chasmDefIndices.find(voxel) == this->chasmDefIndices.end());
	this->chasmDefIndices.emplace(voxel, id);
}

void VoxelChunk::addDoorAnimInst(VoxelDoorAnimationInstance &&animInst)
{
	this->doorAnimInsts.emplace_back(std::move(animInst));
}

void VoxelChunk::addFadeAnimInst(VoxelFadeAnimationInstance &&animInst)
{
	this->fadeAnimInsts.emplace_back(std::move(animInst));
}

void VoxelChunk::addChasmWallInst(VoxelChasmWallInstance &&inst)
{
	this->chasmWallInsts.emplace_back(std::move(inst));
}

void VoxelChunk::addDoorVisibilityInst(VoxelDoorVisibilityInstance &&inst)
{
	this->doorVisInsts.emplace_back(std::move(inst));
}

void VoxelChunk::addTriggerInst(VoxelTriggerInstance &&inst)
{
	this->triggerInsts.emplace_back(std::move(inst));
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

void VoxelChunk::setMeshDefDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyMeshDefPositions, VoxelDirtyType::MeshDefinition);
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

void VoxelChunk::setChasmWallInstDirty(SNInt x, int y, WEInt z)
{
	this->trySetVoxelDirtyInternal(x, y, z, this->dirtyChasmWallInstPositions, VoxelDirtyType::ChasmWall);
}

void VoxelChunk::clear()
{
	Chunk::clear();
	this->meshDefs.clear();
	this->textureDefs.clear();
	this->traitsDefs.clear();
	this->transitionDefs.clear();
	this->triggerDefs.clear();
	this->lockDefs.clear();
	this->buildingNames.clear();
	this->doorDefs.clear();
	this->chasmDefs.clear();
	this->meshDefIDs.clear();
	this->textureDefIDs.clear();
	this->traitsDefIDs.clear();
	this->dirtyVoxelTypes.clear();
	this->dirtyMeshDefPositions.clear();
	this->dirtyDoorAnimInstPositions.clear();
	this->dirtyDoorVisInstPositions.clear();
	this->dirtyFadeAnimInstPositions.clear();
	this->dirtyChasmWallInstPositions.clear();
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
}

void VoxelChunk::clearDirtyVoxels()
{
	this->dirtyVoxelTypes.fill(static_cast<VoxelDirtyType>(0));
	this->dirtyMeshDefPositions.clear();
	this->dirtyDoorAnimInstPositions.clear();
	this->dirtyDoorVisInstPositions.clear();
	this->dirtyFadeAnimInstPositions.clear();
	this->dirtyChasmWallInstPositions.clear();
}

void VoxelChunk::update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager)
{
	const ChunkInt2 &chunkPos = this->getPosition();

	// Update doors.
	for (int i = static_cast<int>(this->doorAnimInsts.size()) - 1; i >= 0; i--)
	{
		VoxelDoorAnimationInstance &animInst = this->doorAnimInsts[i];
		animInst.update(dt);

		const VoxelInt3 voxel(animInst.x, animInst.y, animInst.z);
		if (animInst.stateType != VoxelDoorAnimationInstance::StateType::Closed)
		{
			if (animInst.stateType != VoxelDoorAnimationInstance::StateType::Closing)
			{
				// If the player is far enough away, set the door to closing and play the on-closing sound at the center of
				// the voxel if it is defined for the door.
				const CoordDouble3 voxelCoord(chunkPos, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
				const VoxelDouble3 diff = playerCoord - voxelCoord;

				constexpr double closeDistSqr = ArenaLevelUtils::DOOR_CLOSE_DISTANCE * ArenaLevelUtils::DOOR_CLOSE_DISTANCE;
				const double distSqr = diff.lengthSquared();

				if (distSqr >= closeDistSqr)
				{
					animInst.setStateType(VoxelDoorAnimationInstance::StateType::Closing);

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
						const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(voxelCoord);
						audioManager.playSound(closeSoundDef.soundFilename, absoluteSoundPosition);
					}
				}
			}
		}
		else
		{
			// Play closed sound if it is defined for the door.
			VoxelChunk::DoorDefID doorDefID;
			if (!this->tryGetDoorDefID(animInst.x, animInst.y, animInst.z, &doorDefID))
			{
				DebugCrash("Expected door def ID to exist.");
			}

			const DoorDefinition &doorDef = this->getDoorDef(doorDefID);
			const DoorDefinition::CloseSoundDef &closeSoundDef = doorDef.getCloseSound();
			if (closeSoundDef.closeType == DoorDefinition::CloseType::OnClosed)
			{
				const CoordDouble3 soundCoord(chunkPos, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
				const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
				audioManager.playSound(closeSoundDef.soundFilename, absoluteSoundPosition);
			}

			this->doorAnimInsts.erase(this->doorAnimInsts.begin() + i);
		}

		this->setDoorAnimInstDirty(voxel.x, voxel.y, voxel.z);
	}

	// Update fading voxels.
	for (int i = static_cast<int>(this->fadeAnimInsts.size()) - 1; i >= 0; i--)
	{
		VoxelFadeAnimationInstance &animInst = this->fadeAnimInsts[i];
		animInst.update(dt);

		const VoxelInt3 voxel(animInst.x, animInst.y, animInst.z);
		if (animInst.isDoneFading())
		{
			// Convert the faded voxel to air or a chasm depending on the Y coordinate.
			const bool isChasm = voxel.y == 0; // @todo: check if there's a chasm def here instead
			if (isChasm)
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
				this->setMeshDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_MESH_DEF_ID);
				this->setTextureDefID(voxel.x, voxel.y, voxel.z, VoxelChunk::AIR_TEXTURE_DEF_ID);
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

			this->fadeAnimInsts.erase(this->fadeAnimInsts.begin() + i);
		}
		else
		{
			this->setFadeAnimInstDirty(voxel.x, voxel.y, voxel.z);
		}
	}
}
