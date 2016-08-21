#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>

#include "Voxel.h"

// A chunk is essentially a 3D array, like in Minecraft. Every chunk will be stored 
// in the world's 2D array of chunks. No offset needed then, as the (x, z) offset 
// is only needed when the world doesn't use a discrete storage container.

class Chunk
{
private:
	static const int32_t Width = 8;
	static const int32_t Height = 4;
	static const int32_t Depth = 8;
	static const int32_t MaxVolume = Chunk::Width * Chunk::Height * Chunk::Depth;

	std::array<Voxel, Chunk::MaxVolume> voxels;
public:
	// Initializes all voxels to the given voxel.
	Chunk(const Voxel &fillVoxel);

	// Initializes all voxels to empty.
	Chunk();
	~Chunk();

	const Voxel &get(int32_t x, int32_t y, int32_t z);
	void set(int32_t x, int32_t y, int32_t z, const Voxel &voxel);
};

#endif
