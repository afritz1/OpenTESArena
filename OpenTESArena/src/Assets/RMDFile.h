#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <vector>

#include "ArenaTypes.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView2D.h"

// Wilderness map data.
class RMDFile
{
private:
	static constexpr int BYTES_PER_FLOOR = 8192;

	Buffer2D<ArenaVoxelID> flor, map1, map2;
public:
	bool init(const char *filename);

	static constexpr WEInt WIDTH = 64;
	static constexpr SNInt DEPTH = WIDTH;
	static constexpr int ELEMENTS_PER_FLOOR = BYTES_PER_FLOOR / sizeof(ArenaVoxelID);

	// Get voxel data for each floor.
	BufferView2D<const ArenaVoxelID> getFLOR() const;
	BufferView2D<const ArenaVoxelID> getMAP1() const;
	BufferView2D<const ArenaVoxelID> getMAP2() const;
};

#endif
