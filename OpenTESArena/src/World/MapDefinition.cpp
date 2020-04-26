#include "MapDefinition.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"

#include "components/debug/Debug.h"

MapDefinition::MapDefinition()
{
	this->width = 0;
	this->depth = 0;
	this->startLevelIndex = -1;
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

const LevelDefinition &MapDefinition::getLevel(int index) const
{
	return this->levels.get(index);
}
