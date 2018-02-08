#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <cstdint>
#include <string>
#include <vector>

class RMDFile
{
private:
	std::vector<uint8_t> flor, map1, map2;
public:
	RMDFile(const std::string &filename);
	~RMDFile();

	// Constant .RMD dimensions, always 64x64.
	static const int WIDTH;
	static const int DEPTH;

	// Get voxel data for each floor. Each should be 8192 bytes.
	const std::vector<uint8_t> &getFLOR() const;
	const std::vector<uint8_t> &getMAP1() const;
	const std::vector<uint8_t> &getMAP2() const;
};

#endif
