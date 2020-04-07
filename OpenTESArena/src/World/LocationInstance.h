#ifndef LOCATION_INSTANCE_H
#define LOCATION_INSTANCE_H

#include <string>

#include "LocationDefinition.h"

class LocationInstance
{
private:
	std::string nameOverride; // Useful for quest dungeons.
	int locationDefIndex; // Index in province location definitions.
	bool visible;

	// Whether the location instance's name overrides the location definition's.
	bool hasNameOverride() const;
public:
	void init(int locationDefIndex, const LocationDefinition &locationDef);

	// Gets the index of the location's definition in its province definition.
	int getLocationDefIndex() const;

	// Whether the location is visible in the province map.
	bool isVisible() const;

	// Gets the location instance's name if it overrides its location definition's,
	// otherwise defaults to the location definition's.
	const std::string &getName(const LocationDefinition &locationDef) const;

	// Toggles location visibility on the world map.
	void toggleVisibility();

	// Sets the location's name override. If empty, then the location definition's name
	// must be used instead.
	void setNameOverride(std::string &&nameOverride);
};

#endif
