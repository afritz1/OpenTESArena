#ifndef VOXEL_GEOMETRY_H
#define VOXEL_GEOMETRY_H

#include "VoxelData.h"
#include "../Math/Quad.h"
#include "../Math/Vector3.h"

// Mostly intended for collision geometry, not rendering.

namespace VoxelGeometry
{
	// Maximum possible quads for one voxel (intended for constexpr array initialization).
	static constexpr int MAX_QUADS = 6;

	// Returns number of quads for the given voxel definition.
	void getInfo(const VoxelData &voxelData, int *outQuadCount);

	// Writes out quads for the given voxel definition with the given origin offset in world space.
	// Returns whether all data was written.
	bool tryGetData(const VoxelData &voxelData, const Int3 &voxel, double ceilingHeight,
		int quadBufferSize, Quad *outQuads, int *outQuadCount);
	bool tryGetData(const VoxelData &voxelData, double ceilingHeight, int quadBufferSize,
		Quad *outQuads, int *outQuadCount);
}

#endif
