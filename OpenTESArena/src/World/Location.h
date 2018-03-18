#ifndef LOCATION_H
#define LOCATION_H

#include <string>

// Each location is a unique city, town, village, or dungeon.

// There are a couple locations with duplicate names, so to get a unique location, 
// it must be paired with a province and a local ID.

enum class ClimateType;
enum class LocationType;

struct Location
{
	std::string name;
	int provinceID;
	LocationType locationType;
	ClimateType climateType;

	Location(const std::string &name, int provinceID,
		LocationType locationType, ClimateType climateType);
	Location();
};

#endif
