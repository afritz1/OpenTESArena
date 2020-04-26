#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "VoxelDefinition.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

class LevelDefinition
{
public:
	// 2 bytes per voxel because the map might be bigger than a chunk.
	using VoxelID = uint16_t;
private:
	Buffer3D<VoxelID> voxels; // Points into voxel definitions list.
	std::vector<VoxelDefinition> voxelDefs;
	// @todo: entity lists
	// @todo: locks
	// @todo: text + sound triggers
public:
	void init(WEInt width, int height, SNInt depth);

	// Width and depth are the same as the map the level is in.
	WEInt getWidth() const;
	int getHeight() const;
	SNInt getDepth() const;

	VoxelID getVoxel(WEInt x, int y, SNInt z) const;
	void setVoxel(WEInt x, int y, SNInt z, VoxelID voxel);
};

#endif
