#ifndef LOCATION_H
#define LOCATION_H

#include <string>
#include <vector>

#include "LocationName.h"

// I solved the ambiguous location name problem by adding a number after the
// ambiguous ones; i.e., "Riverbridge2" and "Glenpoint2". They refer to the
// Morrowind and Valenwood locations, respectively.

enum class ClimateName;
enum class LocationType;

class Location
{
private:
	LocationName locationName;
public:
	Location(LocationName locationName);
	~Location();

	static std::vector<LocationName> getAllLocations();

	const LocationName &getLocationName() const;
	LocationType getLocationType() const;
	ClimateName getClimateName() const;
	std::string toString() const;
	std::string typeToString() const;

	// Maybe "getMapCoordinates()" would get the pixel point? The world coordinates 
	// could be a function of that. Would it mean that all locations sit right on the 
	// corner of their chunk? They should be centered as close as possible.
};

#endif