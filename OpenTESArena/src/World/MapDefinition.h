#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <vector>

#include "LevelDefinition.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer.h"

// Modern replacement for .MIF files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine, so that it doesn't care about things like
// chunks.

// Not retaining the original game's limitation of all of a map's levels being the same dimensions.

class MIFFile;

class MapDefinition
{
private:
	Buffer<LevelDefinition> levels;
	std::vector<LevelDouble2> startPoints;
	int startLevelIndex;
public:
	MapDefinition();

	void init(const MIFFile &mif);

	int getStartLevelIndex() const;
	int getLevelCount() const;
	const LevelDefinition &getLevel(int index) const;
};

#endif
