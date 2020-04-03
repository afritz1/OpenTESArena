#include "LocationDefinition.h"

#include "components/debug/Debug.h"

void LocationDefinition::CityDefinition::init(CityDefinition::Type type, uint32_t citySeed,
	uint32_t wildSeed, uint32_t provinceSeed, uint32_t rulerSeed, uint32_t distantSkySeed,
	bool coastal, bool premade)
{
	this->type = type;
	this->citySeed = citySeed;
	this->wildSeed = wildSeed;
	this->provinceSeed = provinceSeed;
	this->rulerSeed = rulerSeed;
	this->distantSkySeed = distantSkySeed;
	this->coastal = coastal;
	this->premade = premade;
}

uint32_t LocationDefinition::CityDefinition::getWildDungeonSeed(int wildBlockX, int wildBlockY) const
{
	return (this->provinceSeed + (((wildBlockY << 6) + wildBlockX) & 0xFFFF)) & 0xFFFFFFFF;
}

void LocationDefinition::DungeonDefinition::init()
{
	// Do nothing
}

void LocationDefinition::MainQuestDungeonDefinition::init(MainQuestDungeonDefinition::Type type)
{
	this->type = type;
}

void LocationDefinition::init(LocationDefinition::Type type,
	const CityDataFile::ProvinceData::LocationData &locationData)
{
	this->name = locationData.name;
	this->x = locationData.x;
	this->y = locationData.y;
	this->visibleByDefault = (locationData.name.size() > 0) && (type == LocationDefinition::Type::City);
	this->type = type;
}

void LocationDefinition::initCity(int localCityID, int provinceID, bool coastal, bool premade,
	CityDefinition::Type type, const CityDataFile &cityData)
{
	const auto &provinceData = cityData.getProvinceData(provinceID);
	const auto &locationData = provinceData.getLocationData(localCityID);
	this->init(LocationDefinition::Type::City, locationData);

	const uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);
	const uint32_t wildSeed = cityData.getWildernessSeed(localCityID, provinceID);
	const uint32_t provinceSeed = cityData.getProvinceSeed(provinceID);
	const uint32_t rulerSeed = cityData.getRulerSeed(localCityID, provinceID);
	const uint32_t distantSkySeed = cityData.getDistantSkySeed(localCityID, provinceID);
	this->city.init(type, citySeed, wildSeed, provinceSeed, rulerSeed, distantSkySeed, coastal, premade);
}

void LocationDefinition::initDungeon(const CityDataFile::ProvinceData::LocationData &locationData)
{
	this->init(LocationDefinition::Type::Dungeon, locationData);
	this->dungeon.init();
}

void LocationDefinition::initMainQuestDungeon(MainQuestDungeonDefinition::Type type,
	const CityDataFile::ProvinceData::LocationData &locationData)
{
	this->init(LocationDefinition::Type::MainQuestDungeon, locationData);
	this->mainQuest.init(type);
}

const std::string &LocationDefinition::getName() const
{
	return this->name;
}

int LocationDefinition::getScreenX() const
{
	return this->x;
}

int LocationDefinition::getScreenY() const
{
	return this->y;
}

bool LocationDefinition::isVisibleByDefault() const
{
	return this->visibleByDefault;
}

LocationDefinition::Type LocationDefinition::getType() const
{
	return this->type;
}

const LocationDefinition::CityDefinition &LocationDefinition::getCityDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::City);
	return this->city;
}

const LocationDefinition::DungeonDefinition &LocationDefinition::getDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::Dungeon);
	return this->dungeon;
}

const LocationDefinition::MainQuestDungeonDefinition &LocationDefinition::getMainQuestDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::MainQuestDungeon);
	return this->mainQuest;
}
