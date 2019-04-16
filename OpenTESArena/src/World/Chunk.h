#ifndef CHUNK_H
#define CHUNK_H

#include <array>

// A chunk is a 3D set of voxels for each part of Arena's world, for both interiors and exteriors.
// It's a little odd that interiors are presented as chunks in the original game because it allows
// the player to go into an expanse of nothingness/repeating data if they manage to bypass the
// level perimeter.

template <size_t T>
class Chunk
{
private:
	static constexpr int WIDTH = 64;
	static constexpr int HEIGHT = T;
	static constexpr int DEPTH = WIDTH;

	// Indices into voxel data.
	std::array<uint16_t, WIDTH * HEIGHT * DEPTH> voxels;

	// Chunk coordinates.
	int x, y;

	constexpr int getIndex(int x, int y, int z) const;
public:
	Chunk(int x, int y);

	int getX() const;
	int getY() const;
	constexpr int getWidth() const;
	constexpr int getHeight() const;
	constexpr int getDepth() const;
	uint16_t get(int x, int y, int z) const;
	void set(int x, int y, int z, uint16_t value);
};

// Template instantiations at end of .cpp file.

// Interior chunks are always three voxels high (ground, main floor, ceiling). Exteriors are
// higher to allow for tall buildings.
using InteriorChunk = Chunk<3>;
using ExteriorChunk = Chunk<6>;

#endif
