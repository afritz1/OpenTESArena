#ifndef RMD_FILE_H
#define RMD_FILE_H

#include <array>
#include <cstdint>
#include <string>

class RMDFile
{
public:
	typedef std::array<uint8_t, 8192> ArrayType;
private:
	RMDFile::ArrayType flor, map1, map2;
public:
	RMDFile(const std::string &filename);
	~RMDFile();

	// Constant .RMD dimensions, always 64x64.
	static const int WIDTH;
	static const int DEPTH;

	// Get voxel data for each floor. Each should be 8192 bytes.
	const RMDFile::ArrayType &getFLOR() const;
	const RMDFile::ArrayType &getMAP1() const;
	const RMDFile::ArrayType &getMAP2() const;
};

#endif
