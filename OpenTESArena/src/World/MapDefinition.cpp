#include "MapDefinition.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"

#include "components/debug/Debug.h"

int MapDefinition::Level::getHeight() const
{
	return this->voxels.getHeight();
}

MapDefinition::VoxelID MapDefinition::Level::getVoxel(WEInt x, int y, SNInt z) const
{
	return this->voxels.get(x, y, z);
}

void MapDefinition::Level::setVoxel(WEInt x, int y, SNInt z, MapDefinition::VoxelID voxel)
{
	this->voxels.set(x, y, z, voxel);
}

void MapDefinition::Level::init(WEInt width, int height, SNInt depth)
{
	this->voxels.init(width, height, depth);
}

MapDefinition::MapDefinition()
{
	this->width = 0;
	this->depth = 0;
	this->startLevelIndex = 0;
}

void MapDefinition::init(const MIFFile &mif)
{
	DebugNotImplemented();
}

void MapDefinition::init(const RMDFile &rmd)
{
	DebugNotImplemented();
}

WEInt MapDefinition::getWidth() const
{
	return this->width;
}

SNInt MapDefinition::getDepth() const
{
	return this->depth;
}

int MapDefinition::getStartLevelIndex() const
{
	return this->startLevelIndex;
}

int MapDefinition::getLevelCount() const
{
	return static_cast<int>(this->levels.getCount());
}

const MapDefinition::Level &MapDefinition::getLevel(int index) const
{
	return this->levels.get(index);
}
