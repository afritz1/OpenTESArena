#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <vector>

#include "LevelDefinition.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer.h"

// Modern replacement for .MIF files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine, so that it doesn't care about things like
// chunks.

class MIFFile;

class MapDefinition
{
private:
	Buffer<LevelDefinition> levels;
	std::vector<LevelDouble2> startPoints;
	SNInt width;
	WEInt depth;
	int startLevelIndex;
public:
	MapDefinition();

	void init(const MIFFile &mif);

	SNInt getWidth() const;
	WEInt getDepth() const;
	int getStartLevelIndex() const;
	int getLevelCount() const;
	const LevelDefinition &getLevel(int index) const;
};

#endif
