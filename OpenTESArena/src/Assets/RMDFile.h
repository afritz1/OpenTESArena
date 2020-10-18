#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <vector>

#include "ArenaTypes.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView2D.h"

class RMDFile
{
private:
	static constexpr int BYTES_PER_FLOOR = 8192;

	Buffer2D<ArenaTypes::VoxelID> flor, map1, map2;
public:
	bool init(const char *filename);

	static constexpr WEInt WIDTH = 64;
	static constexpr SNInt DEPTH = WIDTH;
	static constexpr int ELEMENTS_PER_FLOOR = BYTES_PER_FLOOR / sizeof(ArenaTypes::VoxelID);

	// Get voxel data for each floor.
	BufferView2D<const ArenaTypes::VoxelID> getFLOR() const;
	BufferView2D<const ArenaTypes::VoxelID> getMAP1() const;
	BufferView2D<const ArenaTypes::VoxelID> getMAP2() const;
};

#endif
