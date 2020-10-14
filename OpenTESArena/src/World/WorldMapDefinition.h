#ifndef WORLD_MAP_DEFINITION_H
#define WORLD_MAP_DEFINITION_H

#include <vector>

#include "ProvinceDefinition.h"

class BinaryAssetLibrary;

class WorldMapDefinition
{
private:
	std::vector<ProvinceDefinition> provinces;
public:
	// Initialize from original game data.
	void init(const BinaryAssetLibrary &binaryAssetLibrary);
	// @todo: eventually have init(const char *filename) for custom world maps.

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	// Gets the province definition at the given index.
	const ProvinceDefinition &getProvinceDef(int index) const;

	// Attempts to get the index of the given province definition in the world map.
	bool tryGetProvinceIndex(const ProvinceDefinition &provinceDef, int *outProvinceIndex) const;
};

#endif
