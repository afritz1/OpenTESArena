#ifndef PROVINCE_DEFINITION_H
#define PROVINCE_DEFINITION_H

#include <vector>

#include "LocationDefinition.h"
#include "../Math/Rect.h"

class BinaryAssetLibrary;

class ProvinceDefinition
{
private:
	std::vector<LocationDefinition> locations;
	std::string name;
	int globalX, globalY, globalW, globalH; // Province-to-world-map projection.
	int raceID;
	bool animatedDistantLand;
public:
	// Initialize from original game data.
	void init(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary);
	// @todo: eventually have init(const char *filename) for custom provinces.

	// Gets the number of locations in the province.
	int getLocationCount() const;

	// Gets the location definition at the given index.
	const LocationDefinition &getLocationDef(int index) const;

	// Gets the display name of the province.
	const std::string &getName() const;
	
	// Creates a rectangle from the province's global X, Y, W, and H values.
	Rect getGlobalRect() const;

	// Gets the race ID that refers to a specific race.
	int getRaceID() const;

	// Whether the province contains any animated distant land like volcanoes.
	// @todo: expand this into an actual data structure
	bool hasAnimatedDistantLand() const;

	// Returns whether the two definitions reference the same province in the world map.
	bool matches(const ProvinceDefinition &other) const;

	// Attempts to get the index of the given location definition in the province.
	bool tryGetLocationIndex(const LocationDefinition &locationDef, int *outLocationIndex) const;
};

#endif
