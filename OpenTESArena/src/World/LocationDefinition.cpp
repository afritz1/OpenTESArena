#include "LocationDefinition.h"

#include "components/debug/Debug.h"

void LocationDefinition::CityDefinition::init(CityDefinition::Type type, bool coastal, bool premade)
{
	this->type = type;
	this->coastal = coastal;
	this->premade = premade;
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
	this->visibleByDefault = locationData.isVisible();
	this->type = type;
}

void LocationDefinition::initCity(CityDefinition::Type type,
	const CityDataFile::ProvinceData::LocationData &locationData, bool coastal, bool premade)
{
	this->init(LocationDefinition::Type::City, locationData);
	this->city.init(type, coastal, premade);
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
