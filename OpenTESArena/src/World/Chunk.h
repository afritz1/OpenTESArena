#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <climits>
#include <cstdint>
#include <limits>
#include <vector>

#include "ChunkUtils.h"
#include "VoxelDefinition.h"
#include "VoxelInstance.h"
#include "VoxelUtils.h"
#include "../Math/MathUtils.h"

#include "components/utilities/Buffer3D.h"

// A 3D set of voxels for a portion of the game world.

class Chunk
{
public:
	// This type must always support at least as many bits as are needed per voxel.
	using VoxelID = uint8_t;
private:
	// Determines the allowed number of voxel types per chunk.
	static constexpr int BITS_PER_VOXEL = 8;
	static_assert((sizeof(VoxelID) * CHAR_BIT) >= BITS_PER_VOXEL);

	static constexpr int MAX_VOXEL_DEFS = 1 << BITS_PER_VOXEL;
	static constexpr VoxelID AIR_VOXEL_ID = 0;

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
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	void init(const ChunkInt2 &coord, int height);

	int getHeight() const;

	// Gets the chunk's XY coordinate in the world.
	const ChunkInt2 &getCoord() const;

	// Gets the voxel ID at the given coordinate.
	VoxelID getVoxel(SNInt x, int y, WEInt z) const;

	// Gets the number of active voxel definitions.
	int getVoxelDefCount() const;

	// Gets the voxel definition associated with a voxel ID.
	const VoxelDefinition &getVoxelDef(VoxelID id) const;

	// Gets the number of voxel instances.
	int getVoxelInstCount() const;

	// Gets the voxel instance at the given index.
	VoxelInstance &getVoxelInst(int index);
	const VoxelInstance &getVoxelInst(int index) const;

	// Sets the voxel at the given coordinate.
	void setVoxel(SNInt x, int y, WEInt z, VoxelID id);

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
