#ifndef LOCATION_INSTANCE_H
#define LOCATION_INSTANCE_H

#include "LocationDefinition.h"

class LocationInstance
{
private:
	int locationDefIndex; // Index in province location definitions.
	bool visible;
public:
	void init(int locationDefIndex, const LocationDefinition &locationDef);

	// Gets the index of the location's definition in its province definition.
	int getLocationDefIndex() const;

	// Whether the location is visible in the province map.
	bool isVisible() const;

	void toggleVisibility();
};

#endif
