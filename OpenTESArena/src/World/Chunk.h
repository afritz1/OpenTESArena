#ifndef CHUNK_H
#define CHUNK_H

#include <array>

#include "Voxel.h"

// A chunk is essentially a 3D array. Every chunk will be stored in the world's 2D 
// array of chunks. No offset needed then, as the (x, z) offset is only needed when 
// the world doesn't use a discrete storage container.

class Chunk
{
private:
	static const int Width = 8;
	static const int Height = 4;
	static const int Depth = 8;
	static const int MaxVolume = Chunk::Width * Chunk::Height * Chunk::Depth;

	std::array<Voxel, Chunk::MaxVolume> voxels;
public:
	// Initializes all voxels to the given voxel.
	Chunk(const Voxel &fillVoxel);

	// Initializes all voxels to empty.
	Chunk();
	~Chunk();

	const Voxel &get(int x, int y, int z);
	void set(int x, int y, int z, const Voxel &voxel);
};

#endif