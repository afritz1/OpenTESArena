#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <vector>

#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView2D.h"

class RMDFile
{
public:
	using VoxelID = uint16_t;
private:
	static constexpr int BYTES_PER_FLOOR = 8192;

	Buffer2D<VoxelID> flor, map1, map2;
public:
	bool init(const char *filename);

	static constexpr WEInt WIDTH = 64;
	static constexpr SNInt DEPTH = WIDTH;
	static constexpr int ELEMENTS_PER_FLOOR = BYTES_PER_FLOOR / sizeof(VoxelID);

	// Get voxel data for each floor.
	BufferView2D<const VoxelID> getFLOR() const;
	BufferView2D<const VoxelID> getMAP1() const;
	BufferView2D<const VoxelID> getMAP2() const;
};

#endif
