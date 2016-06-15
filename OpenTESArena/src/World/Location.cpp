#include <cassert>
#include <map>

#include "Location.h"

#include "ClimateName.h"
#include "LocationType.h"
#include "ProvinceName.h"

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

Location::Location(const std::string &name, ProvinceName provinceName, 
	LocationType locationType, ClimateName climateName)
{
	this->name = name;
	this->provinceName = provinceName;
	this->locationType = locationType;
	this->climateName = climateName;
}

Location::~Location()
{

}

ProvinceName Location::getProvinceName() const
{
	return this->provinceName;
}

LocationType Location::getLocationType() const
{	
	return this->locationType;
}

ClimateName Location::getClimateName() const
{
	return this->climateName;
}

const std::string &Location::getDisplayName() const
{
	return this->name;
}

std::string Location::typeToString() const
{
	auto displayName = LocationTypeDisplayNames.at(this->getLocationType());
	return displayName;
}
