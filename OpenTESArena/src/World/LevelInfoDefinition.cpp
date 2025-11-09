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
	this->addVoxelShapeDef(VoxelShapeDefinition());
	this->addVoxelTextureDef(VoxelTextureDefinition());
	this->addVoxelShadingDef(VoxelShadingDefinition());
	this->addVoxelTraitsDef(VoxelTraitsDefinition());
}

int LevelInfoDefinition::getVoxelShapeDefCount() const
{
	return static_cast<int>(this->voxelShapeDefs.size());
}

int LevelInfoDefinition::getVoxelTextureDefCount() const
{
	return static_cast<int>(this->voxelTextureDefs.size());
}

int LevelInfoDefinition::getVoxelShadingDefCount() const
{
	return static_cast<int>(this->voxelShadingDefs.size());
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

const VoxelShapeDefinition &LevelInfoDefinition::getVoxelShapeDef(LevelVoxelShapeDefID id) const
{
	DebugAssertIndex(this->voxelShapeDefs, id);
	return this->voxelShapeDefs[id];
}

const VoxelTextureDefinition &LevelInfoDefinition::getVoxelTextureDef(LevelVoxelTextureDefID id) const
{
	DebugAssertIndex(this->voxelTextureDefs, id);
	return this->voxelTextureDefs[id];
}

const VoxelShadingDefinition &LevelInfoDefinition::getVoxelShadingDef(LevelVoxelShadingDefID id) const
{
	DebugAssertIndex(this->voxelShadingDefs, id);
	return this->voxelShadingDefs[id];
}

const VoxelTraitsDefinition &LevelInfoDefinition::getVoxelTraitsDef(LevelVoxelTraitsDefID id) const
{
	DebugAssertIndex(this->voxelTraitsDefs, id);
	return this->voxelTraitsDefs[id];
}

const EntityDefinition &LevelInfoDefinition::getEntityDef(LevelVoxelEntityDefID id) const
{
	DebugAssertIndex(this->entityDefs, id);
	return this->entityDefs[id];
}

const LockDefinition &LevelInfoDefinition::getLockDef(LevelVoxelLockDefID id) const
{
	DebugAssertIndex(this->lockDefs, id);
	return this->lockDefs[id];
}

const VoxelTriggerDefinition &LevelInfoDefinition::getTriggerDef(LevelVoxelTriggerDefID id) const
{
	DebugAssertIndex(this->triggerDefs, id);
	return this->triggerDefs[id];
}

const TransitionDefinition &LevelInfoDefinition::getTransitionDef(LevelVoxelTransitionDefID id) const
{
	DebugAssertIndex(this->transitionDefs, id);
	return this->transitionDefs[id];
}

const std::string &LevelInfoDefinition::getBuildingName(LevelVoxelBuildingNameID id) const
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

const VoxelDoorDefinition &LevelInfoDefinition::getDoorDef(LevelVoxelDoorDefID id) const
{
	DebugAssertIndex(this->doorDefs, id);
	return this->doorDefs[id];
}

const VoxelChasmDefinition &LevelInfoDefinition::getChasmDef(LevelVoxelChasmDefID id) const
{
	DebugAssertIndex(this->chasmDefs, id);
	return this->chasmDefs[id];
}

double LevelInfoDefinition::getCeilingScale() const
{
	return this->ceilingScale;
}

LevelVoxelShapeDefID LevelInfoDefinition::addVoxelShapeDef(VoxelShapeDefinition &&def)
{
	const LevelVoxelShapeDefID id = static_cast<LevelVoxelShapeDefID>(this->voxelShapeDefs.size());
	this->voxelShapeDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelTextureDefID LevelInfoDefinition::addVoxelTextureDef(VoxelTextureDefinition &&def)
{
	const LevelVoxelTextureDefID id = static_cast<LevelVoxelTextureDefID>(this->voxelTextureDefs.size());
	this->voxelTextureDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelShadingDefID LevelInfoDefinition::addVoxelShadingDef(VoxelShadingDefinition &&def)
{
	const LevelVoxelShadingDefID id = static_cast<LevelVoxelShadingDefID>(this->voxelShadingDefs.size());
	this->voxelShadingDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelTraitsDefID LevelInfoDefinition::addVoxelTraitsDef(VoxelTraitsDefinition &&def)
{
	const LevelVoxelTraitsDefID id = static_cast<LevelVoxelTraitsDefID>(this->voxelTraitsDefs.size());
	this->voxelTraitsDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelEntityDefID LevelInfoDefinition::addEntityDef(EntityDefinition &&def)
{
	const LevelVoxelEntityDefID id = static_cast<LevelVoxelEntityDefID>(this->entityDefs.size());
	this->entityDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelLockDefID LevelInfoDefinition::addLockDef(LockDefinition &&def)
{
	const LevelVoxelLockDefID id = static_cast<LevelVoxelLockDefID>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelTriggerDefID LevelInfoDefinition::addTriggerDef(VoxelTriggerDefinition &&def)
{
	const LevelVoxelTriggerDefID id = static_cast<LevelVoxelTriggerDefID>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelTransitionDefID LevelInfoDefinition::addTransitionDef(TransitionDefinition &&def)
{
	const LevelVoxelTransitionDefID id = static_cast<LevelVoxelTransitionDefID>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelBuildingNameID LevelInfoDefinition::addBuildingName(std::string &&name)
{
	const LevelVoxelBuildingNameID id = static_cast<LevelVoxelBuildingNameID>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(name));
	return id;
}

LevelVoxelDoorDefID LevelInfoDefinition::addDoorDef(VoxelDoorDefinition &&def)
{
	const LevelVoxelDoorDefID id = static_cast<LevelVoxelDoorDefID>(this->doorDefs.size());
	this->doorDefs.emplace_back(std::move(def));
	return id;
}

LevelVoxelChasmDefID LevelInfoDefinition::addChasmDef(VoxelChasmDefinition &&def)
{
	const LevelVoxelChasmDefID id = static_cast<LevelVoxelChasmDefID>(this->chasmDefs.size());
	this->chasmDefs.emplace_back(std::move(def));
	return id;
}

void LevelInfoDefinition::setLockLevel(LevelVoxelLockDefID id, int lockLevel)
{
	DebugAssertIndex(this->lockDefs, id);
	LockDefinition &lockDef = this->lockDefs[id];
	lockDef.lockLevel = lockLevel;
}

void LevelInfoDefinition::setTransitionInteriorDisplayName(LevelVoxelTransitionDefID id, std::string &&name)
{
	DebugAssertIndex(this->transitionDefs, id);
	TransitionDefinition &transitionDef = this->transitionDefs[id];
	if (transitionDef.type != TransitionType::EnterInterior)
	{
		DebugLogErrorFormat("Transition definition %d does not allow interior display names.", id);
		return;
	}

	MapGenerationInteriorInfo &interiorInfo = transitionDef.interiorEntrance.interiorGenInfo;
	if (interiorInfo.type != MapGenerationInteriorType::Prefab)
	{
		DebugLogErrorFormat("Transition definition %d interior info must be a prefab for interior display name.", id);
		return;
	}

	interiorInfo.prefab.displayName = std::move(name);
}

void LevelInfoDefinition::setBuildingNameOverride(LevelVoxelBuildingNameID id, std::string &&name)
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
