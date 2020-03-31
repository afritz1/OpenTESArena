#ifndef WORLD_MAP_DEFINITION_H
#define WORLD_MAP_DEFINITION_H

#include <vector>

#include "ProvinceDefinition.h"
#include "../Assets/CityDataFile.h"

class WorldMapDefinition
{
private:
	std::vector<ProvinceDefinition> provinces;
public:
	// Initialize from original game data.
	void init(const CityDataFile &cityData);
	// @todo: eventually have init(const char *filename) for custom world maps.

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	// Gets the province definition at the given index.
	const ProvinceDefinition &getProvinceDef(int index) const;

	void clear();
};

#endif
