#include "MapDefinition.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"

#include "components/debug/Debug.h"

MapDefinition::MapDefinition()
{
	this->startLevelIndex = -1;
}

void MapDefinition::init(const MIFFile &mif)
{
	DebugNotImplemented();
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
