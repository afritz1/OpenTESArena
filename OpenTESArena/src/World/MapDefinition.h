#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <cstdint>

#include "VoxelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

// Modern replacement for .MIF files and .RMD files. Helps create a buffer between how the 
// game world is defined and how it's represented in-engine, so that it doesn't care about
// things like chunks or whatever.

class MIFFile;
class RMDFile;

class MapDefinition
{
public:
	// 2 bytes per voxel because the map might be bigger than a chunk.
	using VoxelID = uint16_t;

	// Don't store things like chasm permutations. Unbaked voxels only.
	class Level
	{
	private:
		Buffer3D<VoxelID> voxels;
		// @todo: entity lists
		// @todo: locks
		// @todo: triggers
	public:
		int getHeight() const;
		VoxelID getVoxel(WEInt x, int y, SNInt z) const;
		void setVoxel(WEInt x, int y, SNInt z, VoxelID voxel);

		void init(WEInt width, int height, SNInt depth);
	};
private:
	Buffer<Level> levels;
	std::vector<OriginalInt2> startPoints;
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
	const Level &getLevel(int index) const;
};

#endif
