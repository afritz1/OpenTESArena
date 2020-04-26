#ifndef MAP_INSTANCE_H
#define MAP_INSTANCE_H

#include "LevelInstance.h"

#include "components/utilities/Buffer.h"

// Contains deltas and changed values for the associated map definition's levels.

class MapInstance
{
private:
	Buffer<LevelInstance> levels;
public:
	void init(int levelCount);

	LevelInstance &getLevel(int index);
};

#endif
