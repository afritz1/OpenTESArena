#pragma once

#include <vector>

#include "LocationInstance.h"
#include "ProvinceDefinition.h"

class ProvinceInstance
{
private:
	std::vector<LocationInstance> locations;
	int provinceIndex; // Index in world map province definitions.
public:
	void init(int provinceIndex, const ProvinceDefinition &provinceDef);

	// Gets the index of the province's definition in its world map definition.
	int getProvinceIndex() const;

	// Gets the number of locations in the province.
	int getLocationCount() const;

	// Gets the location instance at the given index.
	LocationInstance &getLocationInstance(int index);
	const LocationInstance &getLocationInstance(int index) const;
};
