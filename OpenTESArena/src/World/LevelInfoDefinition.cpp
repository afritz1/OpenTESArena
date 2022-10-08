#include "LevelInfoDefinition.h"

#include "components/debug/Debug.h"

LevelInfoDefinition::LevelInfoDefinition()
{
	this->ceilingScale = 0.0;
}

void LevelInfoDefinition::init(double ceilingScale)
{
	this->ceilingScale = ceilingScale;
	
	// Add air voxel by default.
	this->addVoxelMeshDef(VoxelMeshDefinition());
	this->addVoxelTextureDef(VoxelTextureDefinition());
	this->addVoxelTraitsDef(VoxelTraitsDefinition());
}

int LevelInfoDefinition::getVoxelMeshDefCount() const
{
	return static_cast<int>(this->voxelMeshDefs.size());
}

int LevelInfoDefinition::getVoxelTextureDefCount() const
{
	return static_cast<int>(this->voxelTextureDefs.size());
}

int LevelInfoDefinition::getVoxelTraitsDefCount() const
{
	return static_cast<int>(this->voxelTraitsDefs.size());
}

int LevelInfoDefinition::getEntityDefCount() const
{
	return static_cast<int>(this->entityDefs.size());
}

int LevelInfoDefinition::getLockDefCount() const
{
	return static_cast<int>(this->lockDefs.size());
}

int LevelInfoDefinition::getTriggerDefCount() const
{
	return static_cast<int>(this->triggerDefs.size());
}

int LevelInfoDefinition::getTransitionDefCount() const
{
	return static_cast<int>(this->transitionDefs.size());
}

int LevelInfoDefinition::getBuildingNameCount() const
{
	return static_cast<int>(this->buildingNames.size());
}

int LevelInfoDefinition::getDoorDefCount() const
{
	return static_cast<int>(this->doorDefs.size());
}

int LevelInfoDefinition::getChasmDefCount() const
{
	return static_cast<int>(this->chasmDefs.size());
}

const VoxelMeshDefinition &LevelInfoDefinition::getVoxelMeshDef(LevelDefinition::VoxelMeshDefID id) const
{
	DebugAssertIndex(this->voxelMeshDefs, id);
	return this->voxelMeshDefs[id];
}

const VoxelTextureDefinition &LevelInfoDefinition::getVoxelTextureDef(LevelDefinition::VoxelTextureDefID id) const
{
	DebugAssertIndex(this->voxelTextureDefs, id);
	return this->voxelTextureDefs[id];
}

const VoxelTraitsDefinition &LevelInfoDefinition::getVoxelTraitsDef(LevelDefinition::VoxelTraitsDefID id) const
{
	DebugAssertIndex(this->voxelTraitsDefs, id);
	return this->voxelTraitsDefs[id];
}

const EntityDefinition &LevelInfoDefinition::getEntityDef(LevelDefinition::EntityDefID id) const
{
	DebugAssertIndex(this->entityDefs, id);
	return this->entityDefs[id];
}

const LockDefinition &LevelInfoDefinition::getLockDef(LevelDefinition::LockDefID id) const
{
	DebugAssertIndex(this->lockDefs, id);
	return this->lockDefs[id];
}

const VoxelTriggerDefinition &LevelInfoDefinition::getTriggerDef(LevelDefinition::TriggerDefID id) const
{
	DebugAssertIndex(this->triggerDefs, id);
	return this->triggerDefs[id];
}

const TransitionDefinition &LevelInfoDefinition::getTransitionDef(LevelDefinition::TransitionDefID id) const
{
	DebugAssertIndex(this->transitionDefs, id);
	return this->transitionDefs[id];
}

const std::string &LevelInfoDefinition::getBuildingName(LevelDefinition::BuildingNameID id) const
{
	const auto iter = this->buildingNameOverrides.find(id);
	if (iter != this->buildingNameOverrides.end())
	{
		return iter->second;
	}
	else
	{
		DebugAssertIndex(this->buildingNames, id);
		return this->buildingNames[id];
	}
}

const DoorDefinition &LevelInfoDefinition::getDoorDef(LevelDefinition::DoorDefID id) const
{
	DebugAssertIndex(this->doorDefs, id);
	return this->doorDefs[id];
}

const ChasmDefinition &LevelInfoDefinition::getChasmDef(LevelDefinition::ChasmDefID id) const
{
	DebugAssertIndex(this->chasmDefs, id);
	return this->chasmDefs[id];
}

double LevelInfoDefinition::getCeilingScale() const
{
	return this->ceilingScale;
}

LevelDefinition::VoxelMeshDefID LevelInfoDefinition::addVoxelMeshDef(VoxelMeshDefinition &&def)
{
	const LevelDefinition::VoxelMeshDefID id = static_cast<LevelDefinition::VoxelMeshDefID>(this->voxelMeshDefs.size());
	this->voxelMeshDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::VoxelTextureDefID LevelInfoDefinition::addVoxelTextureDef(VoxelTextureDefinition &&def)
{
	const LevelDefinition::VoxelTextureDefID id = static_cast<LevelDefinition::VoxelTextureDefID>(this->voxelTextureDefs.size());
	this->voxelTextureDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::VoxelTraitsDefID LevelInfoDefinition::addVoxelTraitsDef(VoxelTraitsDefinition &&def)
{
	const LevelDefinition::VoxelTraitsDefID id = static_cast<LevelDefinition::VoxelTraitsDefID>(this->voxelTraitsDefs.size());
	this->voxelTraitsDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::EntityDefID LevelInfoDefinition::addEntityDef(EntityDefinition &&def)
{
	const LevelDefinition::EntityDefID id = static_cast<LevelDefinition::EntityDefID>(this->entityDefs.size());
	this->entityDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::LockDefID LevelInfoDefinition::addLockDef(LockDefinition &&def)
{
	const LevelDefinition::LockDefID id = static_cast<LevelDefinition::LockDefID>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::TriggerDefID LevelInfoDefinition::addTriggerDef(VoxelTriggerDefinition &&def)
{
	const LevelDefinition::TriggerDefID id = static_cast<LevelDefinition::TriggerDefID>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::TransitionDefID LevelInfoDefinition::addTransitionDef(TransitionDefinition &&def)
{
	const LevelDefinition::TransitionDefID id = static_cast<LevelDefinition::TransitionDefID>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::BuildingNameID LevelInfoDefinition::addBuildingName(std::string &&name)
{
	const LevelDefinition::BuildingNameID id = static_cast<LevelDefinition::BuildingNameID>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(name));
	return id;
}

LevelDefinition::DoorDefID LevelInfoDefinition::addDoorDef(DoorDefinition &&def)
{
	const LevelDefinition::DoorDefID id = static_cast<LevelDefinition::DoorDefID>(this->doorDefs.size());
	this->doorDefs.emplace_back(std::move(def));
	return id;
}

LevelDefinition::ChasmDefID LevelInfoDefinition::addChasmDef(ChasmDefinition &&def)
{
	const LevelDefinition::ChasmDefID id = static_cast<LevelDefinition::ChasmDefID>(this->chasmDefs.size());
	this->chasmDefs.emplace_back(std::move(def));
	return id;
}

void LevelInfoDefinition::setBuildingNameOverride(LevelDefinition::BuildingNameID id, std::string &&name)
{
	const auto iter = this->buildingNameOverrides.find(id);
	if (iter != this->buildingNameOverrides.end())
	{
		iter->second = std::move(name);
	}
	else
	{
		this->buildingNameOverrides.emplace(id, std::move(name));
	}
}
