#include "LevelDefinition.h"

void LevelDefinition::init(const MIFFile::Level &level)
{
	DebugNotImplemented();
}

void LevelDefinition::init(const RMDFile &rmd)
{
	DebugNotImplemented();
}

SNInt LevelDefinition::getWidth() const
{
	return this->voxels.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxels.getHeight();
}

WEInt LevelDefinition::getDepth() const
{
	return this->voxels.getDepth();
}

int LevelDefinition::getVoxelDefCount() const
{
	return static_cast<int>(this->voxelDefs.size());
}

const VoxelDefinition &LevelDefinition::getVoxelDef(VoxelID id) const
{
	DebugAssertIndex(this->voxelDefs, id);
	return this->voxelDefs[id];
}

int LevelDefinition::getEntityPlacementDefCount() const
{
	return static_cast<int>(this->entityPlacementDefs.size());
}

const LevelDefinition::EntityPlacementDef &LevelDefinition::getEntityPlacementDef(int index) const
{
	DebugAssertIndex(this->entityPlacementDefs, index);
	return this->entityPlacementDefs[index];
}

int LevelDefinition::getEntityDefsCount() const
{
	return static_cast<int>(this->entityDefs.size());
}

const EntityDefinition &LevelDefinition::getEntityDef(int index) const
{
	DebugAssertIndex(this->entityDefs, index);
	return this->entityDefs[index];
}

int LevelDefinition::getLockDefsCount() const
{
	return static_cast<int>(this->lockDefs.size());
}

const LockDefinition &LevelDefinition::getLockDef(int index) const
{
	DebugAssertIndex(this->lockDefs, index);
	return this->lockDefs[index];
}

int LevelDefinition::getTriggerDefCount() const
{
	return static_cast<int>(this->triggerDefs.size());
}

const TriggerDefinition &LevelDefinition::getTriggerDef(int index) const
{
	DebugAssertIndex(this->triggerDefs, index);
	return this->triggerDefs[index];
}

LevelDefinition::VoxelID LevelDefinition::getVoxel(SNInt x, int y, WEInt z) const
{
	return this->voxels.get(x, y, z);
}

void LevelDefinition::setVoxel(SNInt x, int y, WEInt z, VoxelID voxel)
{
	this->voxels.set(x, y, z, voxel);
}
