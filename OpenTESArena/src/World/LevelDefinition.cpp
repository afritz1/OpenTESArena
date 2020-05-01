#include "LevelDefinition.h"

void LevelDefinition::init(WEInt width, int height, SNInt depth)
{
	this->voxels.init(width, height, depth);
}

WEInt LevelDefinition::getWidth() const
{
	return this->voxels.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxels.getHeight();
}

SNInt LevelDefinition::getDepth() const
{
	return this->voxels.getDepth();
}

int LevelDefinition::getVoxelDefCount() const
{
	return static_cast<int>(this->voxelDefs.size());
}

const VoxelDefinition &LevelDefinition::getVoxelDef(uint16_t id) const
{
	DebugAssertIndex(this->voxelDefs, id);
	return this->voxelDefs[id];
}

LevelDefinition::VoxelID LevelDefinition::getVoxel(WEInt x, int y, SNInt z) const
{
	return this->voxels.get(x, y, z);
}

void LevelDefinition::setVoxel(WEInt x, int y, SNInt z, VoxelID voxel)
{
	this->voxels.set(x, y, z, voxel);
}
