#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <vector>

class RMDFile
{
public:
	using VoxelID = uint16_t;
private:
	// Bytes per floor, always 8192.
	static const int BYTES_PER_FLOOR;

	// Using vectors because arrays caused stack overflow warnings.
	std::vector<uint16_t> flor, map1, map2;
public:
	bool init(const char *filename);

	// Constant .RMD dimensions, always 64x64.
	static const int WIDTH;
	static const int DEPTH;

	// A function of bytes per floor.
	static const int ELEMENTS_PER_FLOOR;

	// Get voxel data for each floor. Each should be 8192 bytes.
	const std::vector<VoxelID> &getFLOR() const;
	const std::vector<VoxelID> &getMAP1() const;
	const std::vector<VoxelID> &getMAP2() const;
};

#endif
