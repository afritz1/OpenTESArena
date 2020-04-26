#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <vector>

#include "LevelDefinition.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer.h"

// Modern replacement for .MIF files and .RMD files. Helps create a buffer between how the 
// game world is defined and how it's represented in-engine, so that it doesn't care about
// things like chunks or whatever.

class MIFFile;
class RMDFile;

class MapDefinition
{
private:
	Buffer<LevelDefinition> levels;
	std::vector<OriginalDouble2> startPoints;
	WEInt width;
	SNInt depth;
	int startLevelIndex;

	// @todo: might be similar to .MIF files in the sense that a voxel has an ID for either a voxel or entity?

	// @todo: read-only data that defines each level in an "unbaked" format (context-free voxels).
	// @todo: extending old features like entity position in a voxel to be anywhere
	// in the voxel (useful for grass, etc.).
public:
	MapDefinition();

	void init(const MIFFile &mif);
	void init(const RMDFile &rmd);

	WEInt getWidth() const;
	SNInt getDepth() const;
	int getStartLevelIndex() const;
	int getLevelCount() const;
	const LevelDefinition &getLevel(int index) const;
};

#endif
