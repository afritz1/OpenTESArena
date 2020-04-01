#ifndef WORLD_MAP_DEFINITION_H
#define WORLD_MAP_DEFINITION_H

#include <vector>

#include "ProvinceDefinition.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"

class WorldMapDefinition
{
private:
	std::vector<ProvinceDefinition> provinces;
public:
	// Initialize from original game data.
	void init(const CityDataFile &cityData, const ExeData::CityGeneration &cityGen);
	// @todo: eventually have init(const char *filename) for custom world maps.

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	// Gets the province definition at the given index.
	const ProvinceDefinition &getProvinceDef(int index) const;
};

#endif
