#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>

#include "VoxelData.h"

// A chunk is a 3D set of voxels for each part of Arena's world, for both interiors and exteriors.
// It's a little odd that interiors are presented as chunks in the original game because it allows
// the player to go into an expanse of nothingness/repeating data if they manage to bypass the
// level perimeter.

// There should be fewer than 256 unique voxel types per chunk. In my original design with
// voxel grids, each voxel was two bytes because a voxel grid was generally much larger than
// a chunk, encapsulating several more voxel types. But within a single chunk, 256 should work.
using VoxelID = uint8_t;

template <size_t T>
class Chunk
{
private:
	static constexpr int WIDTH = 64;
	static constexpr int HEIGHT = T;
	static constexpr int DEPTH = WIDTH;

	// Indices into voxel data.
	std::array<VoxelID, WIDTH * HEIGHT * DEPTH> voxels;

	// Voxel data definitions, pointed to by voxel IDs.
	std::array<VoxelData, 256> voxelData;
	int voxelDataCount;

	// Chunk coordinates.
	int x, y;

	constexpr bool coordIsValid(int x, int y, int z) const;
	constexpr int getIndex(int x, int y, int z) const;
public:
	Chunk(int x, int y);

	int getX() const;
	int getY() const;
	constexpr int getWidth() const;
	constexpr int getHeight() const;
	constexpr int getDepth() const;

	// Gets the voxel ID at the given coordinate.
	constexpr VoxelID get(int x, int y, int z) const;

	// Gets the voxel data associated with a voxel ID.
	const VoxelData &getVoxelData(VoxelID id) const;

	// Sets the voxel at the given coordinate.
	constexpr void set(int x, int y, int z, VoxelID value);

	// Adds a voxel data definition and returns its assigned ID.
	VoxelID addVoxelData(VoxelData &&voxelData);
};

// Template instantiations at end of .cpp file.

// Interior chunks are always three voxels high (ground, main floor, ceiling). Exteriors are
// higher to allow for tall buildings.
using InteriorChunk = Chunk<3>;
using ExteriorChunk = Chunk<6>;

#endif
