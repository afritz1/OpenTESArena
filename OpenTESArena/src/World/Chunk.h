#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>
#include <limits>

#include "VoxelDefinition.h"

#include "components/utilities/Buffer3D.h"

// A chunk is a 3D set of voxels for each part of the world, for both interiors and exteriors.
class Chunk
{
public:
	// There should be fewer than 256 unique voxel types per chunk. If more are needed, then the data
	// can be redesigned to be something like 9 bits per voxel (and the ID type would be 16-bit).
	using VoxelID = uint8_t;
private:
	static constexpr int MAX_VOXEL_DEFS = std::numeric_limits<VoxelID>::max() + 1;

	// Indices into voxel definitions. Size depends on whether it's an interior or exterior.
	Buffer3D<VoxelID> voxels;

	// Voxel definitions, pointed to by voxel IDs. If the associated bool is true,
	// the voxel data is in use by the voxel grid.
	std::array<VoxelDefinition, MAX_VOXEL_DEFS> voxelDefs;
	std::array<bool, MAX_VOXEL_DEFS> activeVoxelDefs;

	// Chunk coordinates.
	int x, y;
protected:
	Chunk(int x, int y, int height);
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

	// Gets the voxel definition associated with a voxel ID.
	const VoxelDefinition &getVoxelDef(Chunk::VoxelID id) const;

	// Gets the number of active voxel definitions.
	int debug_getVoxelDefCount() const;

	// Sets the voxel at the given coordinate.
	void set(int x, int y, int z, VoxelID id);

	// Adds a voxel definition and returns its assigned ID.
	VoxelID addVoxelDef(VoxelDefinition &&voxelDef);

	// Removes a voxel definition so its corresponding voxel ID can be reused.
	void removeVoxelDef(VoxelID id);
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
