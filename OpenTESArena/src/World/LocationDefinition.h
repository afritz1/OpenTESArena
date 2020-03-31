#ifndef LOCATION_DEFINITION_H
#define LOCATION_DEFINITION_H

#include <string>

#include "../Assets/CityDataFile.h"

class LocationDefinition
{
private:
	std::string name;
	int x, y;
	bool visibleByDefault;

	// @todo: LocationDefinitionType? Discriminated union?
	// @todo: Is it a city/town/village? Dungeon? Map dungeon? Staff dungeon?
	// @todo: Coastal?
public:
	// Initialize from original game data.
	void init(const CityDataFile::ProvinceData::LocationData &locationData);
	// @todo: eventually have init(const char *filename) for custom locations.

	// Gets the display name of the location.
	const std::string &getName() const;

	// Pixel coordinates of the location.
	int getScreenX() const;
	int getScreenY() const;

	// Whether the location needs to be discovered for it to be visible on the map.
	bool isVisibleByDefault() const;
};

#endif
