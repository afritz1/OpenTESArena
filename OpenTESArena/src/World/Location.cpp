#include "ClimateType.h"
#include "Location.h"
#include "LocationType.h"

Location::Location(const std::string &name, int provinceID,
	LocationType locationType, ClimateType climateType)
	: name(name)
{
	this->provinceID = provinceID;
	this->locationType = locationType;
	this->climateType = climateType;
}

Location::Location()
	: Location(std::string(), -1, LocationType::CityState, ClimateType::Temperate) { }

Location::~Location()
{

}
