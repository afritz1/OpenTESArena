#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include "VoxelDefinition.h"
#include "VoxelInstance.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

// A 3D set of voxels for a portion of the game world.

class Chunk
{
public:
	// There should be fewer than 256 unique voxel types per chunk. If more are needed, then the data
	// can be redesigned to be something like 9 bits per voxel (and the ID type would be 16-bit).
	using VoxelID = uint8_t;
private:
	static constexpr int MAX_VOXEL_DEFS = std::numeric_limits<VoxelID>::max() + 1;

	// Indices into voxel definitions.
	Buffer3D<VoxelID> voxels;

	// Voxel definitions, pointed to by voxel IDs. If the associated bool is true,
	// the voxel data is in use by the voxel grid.
	std::array<VoxelDefinition, MAX_VOXEL_DEFS> voxelDefs;
	std::array<bool, MAX_VOXEL_DEFS> activeVoxelDefs;

	// Instance data for voxels that are uniquely different in some way.
	std::vector<VoxelInstance> voxelInsts;

	// Chunk coordinates in the world.
	ChunkInt2 coord;
public:
	// Public for some classes that want non-instance dimensions.
	static constexpr int WIDTH = 64;
	static constexpr int DEPTH = WIDTH;

	void init(const ChunkInt2 &coord, int height);

	constexpr int getWidth() const;
	int getHeight() const;
	constexpr int getDepth() const;

	// Gets the chunk's XY coordinate in the world.
	const ChunkInt2 &getCoord() const;

	// Gets the voxel ID at the given coordinate.
	VoxelID get(int x, int y, int z) const;

	// Gets the number of active voxel definitions.
	int getVoxelDefCount() const;

	// Gets the voxel definition associated with a voxel ID.
	const VoxelDefinition &getVoxelDef(Chunk::VoxelID id) const;

	// Gets the number of voxel instances.
	int getVoxelInstCount() const;

	// Gets the voxel instance at the given index.
	VoxelInstance &getVoxelInst(int index);
	const VoxelInstance &getVoxelInst(int index) const;

	// Sets the chunk's XY coordinate in the world.
	void setCoord(const ChunkInt2 &coord);

	// Sets the voxel at the given coordinate.
	void set(int x, int y, int z, VoxelID id);

	// Attempts to add a voxel definition and returns its assigned ID.
	bool tryAddVoxelDef(VoxelDefinition &&voxelDef, VoxelID *outID);

	// Removes a voxel definition so its corresponding voxel ID can be reused.
	void removeVoxelDef(VoxelID id);

	// Clears all chunk state.
	void clear();

	// Animates the chunk's voxels by delta time.
	void update(double dt);
};

#endif
