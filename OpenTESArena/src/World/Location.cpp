#include <cassert>
#include <map>

#include "ClimateType.h"
#include "Location.h"
#include "LocationType.h"

// Display names for each type of location. It's for things like the greeting 
// message when entering a city.
const std::map<LocationType, std::string> LocationTypeDisplayNames =
{
	// Cities.
	{ LocationType::CityState, "City-State" },
	{ LocationType::Town, "Town" },
	{ LocationType::Village, "Village" },

	// Dungeons and main quest.
	{ LocationType::Dungeon, "Dungeon" },
	{ LocationType::Unique, "Unique" }
};

Location::Location(const std::string &name, int provinceID,
	LocationType locationType, ClimateType climateType)
	: name(name)
{
	this->provinceID = provinceID;
	this->locationType = locationType;
	this->climateType = climateType;
}

Location::~Location()
{

}

int Location::getProvinceID() const
{
	return this->provinceID;
}

LocationType Location::getLocationType() const
{
	return this->locationType;
}

ClimateType Location::getClimateType() const
{
	return this->climateType;
}

const std::string &Location::getName() const
{
	return this->name;
}

std::string Location::typeToString() const
{
	auto displayName = LocationTypeDisplayNames.at(this->getLocationType());
	return displayName;
}
