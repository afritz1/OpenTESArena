#include "MapDefinition.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"

#include "components/debug/Debug.h"

void MapDefinition::initInterior(const MIFFile &mif)
{
	// @todo: get .INF filenames from each .MIF level
	DebugNotImplemented();
}

void MapDefinition::initCity(const MIFFile::Level &level, ClimateType climateType,
	WeatherType weatherType)
{
	// @todo: load .INF file for given climate + weather (use CityWorldUtils)
	DebugNotImplemented();
}

void MapDefinition::initWild(const BufferView<const WildBlockID> &wildBlockIDs,
	ClimateType climateType, WeatherType weatherType)
{
	// @todo: load .RMD file for each wild block ID
	// @todo: load .INF file for given climate + weather (use WildWorldUtils)
	DebugNotImplemented();
}

const std::optional<int> &MapDefinition::getStartLevelIndex() const
{
	return this->startLevelIndex;
}

int MapDefinition::getStartPointCount() const
{
	return static_cast<int>(this->startPoints.size());
}

const LevelDouble2 &MapDefinition::getStartPoint(int index) const
{
	DebugAssertIndex(this->startPoints, index);
	return this->startPoints[index];
}

int MapDefinition::getLevelCount() const
{
	return this->levels.getCount();
}

const LevelDefinition &MapDefinition::getLevel(int index) const
{
	return this->levels.get(index);
}

const LevelInfoDefinition &MapDefinition::getLevelInfoForLevel(int levelIndex) const
{
	DebugNotImplemented();
	return this->levelInfos.get(0);
}
