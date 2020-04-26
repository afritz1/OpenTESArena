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

LevelDefinition::VoxelID LevelDefinition::getVoxel(WEInt x, int y, SNInt z) const
{
	return this->voxels.get(x, y, z);
}

void LevelDefinition::setVoxel(WEInt x, int y, SNInt z, VoxelID voxel)
{
	this->voxels.set(x, y, z, voxel);
}
