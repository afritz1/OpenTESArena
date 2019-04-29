#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>
#include <memory>

#include "VoxelData.h"

// There should be fewer than 256 unique voxel types per chunk. If we need more, then the data
// can be redesigned to be something like 9 bits per voxel (and the ID type would be 16-bit).
using VoxelID = uint8_t;

// A chunk is a 3D set of voxels for each part of the world, for both interiors and exteriors.
class Chunk
{
private:
	static constexpr int MAX_VOXEL_DATA = 256;

	// Indices into voxel data.
	std::unique_ptr<VoxelID[]> voxels;

	// Voxel data definitions, pointed to by voxel IDs. If the associated bool is true,
	// the voxel data is in use by the voxel grid.
	std::array<VoxelData, MAX_VOXEL_DATA> voxelData;
	std::array<bool, MAX_VOXEL_DATA> activeVoxelData;

	// Chunk height. Depends on whether it's an interior or exterior.
	int height;

	// Chunk coordinates.
	int x, y;
protected:
	Chunk(int x, int y, int height);
private:
	bool coordIsValid(int x, int y, int z) const;
	int getIndex(int x, int y, int z) const;
public:
	// Public for some classes that want non-instance dimensions.
	static constexpr int WIDTH = 64;
	static constexpr int DEPTH = WIDTH;

	int getX() const;
	int getY() const;
	constexpr int getWidth() const;
	int getHeight() const;
	constexpr int getDepth() const;

	// Gets the voxel ID at the given coordinate.
	VoxelID get(int x, int y, int z) const;

	// Gets the voxel data associated with a voxel ID.
	const VoxelData &getVoxelData(VoxelID id) const;

	// Gets the number of active voxel data definitions.
	int debug_getVoxelDataCount() const;

	// Sets the voxel at the given coordinate.
	void set(int x, int y, int z, VoxelID id);

	// Adds a voxel data definition and returns its assigned ID.
	VoxelID addVoxelData(VoxelData &&voxelData);

	// Removes a voxel data definition so its corresponding voxel ID can be reused.
	void removeVoxelData(VoxelID id);
};

// Interior chunks are always three voxels high (ground, main floor, ceiling).
class InteriorChunk final : public Chunk
{
public:
	InteriorChunk(int x, int y) : Chunk(x, y, 3) { }
};

// Exteriors are higher to allow for tall buildings.
class ExteriorChunk final : public Chunk
{
public:
	ExteriorChunk(int x, int y) : Chunk(x, y, 6) { }
};

#endif
