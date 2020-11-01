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

const MapGeneration::InteriorGenInfo &LevelInfoDefinition::getInteriorGenInfo(
	MapGeneration::InteriorGenInfoID id) const
{
	DebugAssertIndex(this->interiorGenInfos, id);
	return this->interiorGenInfos[id];
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

MapGeneration::InteriorGenInfoID LevelInfoDefinition::addInteriorGenInfo(
	MapGeneration::InteriorGenInfo &&def)
{
	this->interiorGenInfos.emplace_back(std::move(def));
	return static_cast<MapGeneration::InteriorGenInfoID>(this->interiorGenInfos.size()) - 1;
}
