#ifndef MAP_INSTANCE_H
#define MAP_INSTANCE_H

#include "LevelInstance.h"
#include "SkyInstance.h"

#include "components/utilities/Buffer.h"

// Contains instance data for the associated map definition.

class MapInstance
{
private:
	Buffer<LevelInstance> levels;
	Buffer<SkyInstance> skies;
public:
	void init(int levelCount, int skyCount);

	int getLevelCount() const;
	LevelInstance &getLevel(int index);
	
	int getSkyCount() const;
	SkyInstance &getSky(int index);
};

#endif
