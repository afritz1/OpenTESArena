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
	this->addVoxelDef(VoxelDefinition());
}

int LevelInfoDefinition::getVoxelDefCount() const
{
	return static_cast<int>(this->voxelDefs.size());
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

const VoxelDefinition &LevelInfoDefinition::getVoxelDef(LevelDefinition::VoxelDefID id) const
{
	DebugAssertIndex(this->voxelDefs, id);
	return this->voxelDefs[id];
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

const TriggerDefinition &LevelInfoDefinition::getTriggerDef(LevelDefinition::TriggerDefID id) const
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

double LevelInfoDefinition::getCeilingScale() const
{
	return this->ceilingScale;
}

LevelDefinition::VoxelDefID LevelInfoDefinition::addVoxelDef(VoxelDefinition &&def)
{
	this->voxelDefs.emplace_back(std::move(def));
	return static_cast<LevelDefinition::VoxelDefID>(this->voxelDefs.size()) - 1;
}

LevelDefinition::EntityDefID LevelInfoDefinition::addEntityDef(EntityDefinition &&def)
{
	this->entityDefs.emplace_back(std::move(def));
	return static_cast<LevelDefinition::EntityDefID>(this->entityDefs.size()) - 1;
}

LevelDefinition::LockDefID LevelInfoDefinition::addLockDef(LockDefinition &&def)
{
	this->lockDefs.emplace_back(std::move(def));
	return static_cast<LevelDefinition::LockDefID>(this->lockDefs.size()) - 1;
}

LevelDefinition::TriggerDefID LevelInfoDefinition::addTriggerDef(TriggerDefinition &&def)
{
	this->triggerDefs.emplace_back(std::move(def));
	return static_cast<LevelDefinition::TriggerDefID>(this->triggerDefs.size()) - 1;
}

LevelDefinition::TransitionDefID LevelInfoDefinition::addTransitionDef(TransitionDefinition &&def)
{
	this->transitionDefs.emplace_back(std::move(def));
	return static_cast<LevelDefinition::TransitionDefID>(this->transitionDefs.size()) - 1;
}

LevelDefinition::BuildingNameID LevelInfoDefinition::addBuildingName(std::string &&name)
{
	this->buildingNames.emplace_back(std::move(name));
	return static_cast<LevelDefinition::BuildingNameID>(this->buildingNames.size()) - 1;
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
