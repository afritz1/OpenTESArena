#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <vector>

#include "../World/VoxelUtils.h"

class RMDFile
{
public:
	using VoxelID = uint16_t;
private:
	static constexpr int BYTES_PER_FLOOR = 8192;

	// Using vectors because arrays caused stack overflow warnings.
	std::vector<uint16_t> flor, map1, map2;
public:
	bool init(const char *filename);

	static constexpr WEInt WIDTH = 64;
	static constexpr SNInt DEPTH = WIDTH;
	static constexpr int ELEMENTS_PER_FLOOR = BYTES_PER_FLOOR / sizeof(VoxelID);

	// Get voxel data for each floor.
	const std::vector<VoxelID> &getFLOR() const;
	const std::vector<VoxelID> &getMAP1() const;
	const std::vector<VoxelID> &getMAP2() const;
};

#endif
