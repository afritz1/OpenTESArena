#ifndef WORLD_MAP_INSTANCE_H
#define WORLD_MAP_INSTANCE_H

#include <vector>

#include "ProvinceInstance.h"
#include "WorldMapDefinition.h"

class WorldMapInstance
{
private:
	std::vector<ProvinceInstance> provinces;
public:
	void init(const WorldMapDefinition &worldMapDef);

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	// Gets the province instance at the given index.
	ProvinceInstance &getProvinceInstance(int index);
	const ProvinceInstance &getProvinceInstance(int index) const;
};

#endif
