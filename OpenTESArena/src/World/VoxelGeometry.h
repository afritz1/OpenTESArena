#ifndef VOXEL_GEOMETRY_H
#define VOXEL_GEOMETRY_H

#include "VoxelDefinition.h"
#include "../Math/Quad.h"
#include "../Math/Vector3.h"

// Mostly intended for collision geometry, not rendering.

namespace VoxelGeometry
{
	// Maximum possible quads for one voxel (intended for constexpr array initialization).
	static constexpr int MAX_QUADS = 6;

	// Returns number of quads for the given voxel definition.
	void getInfo(const VoxelDefinition &voxelDef, int *outQuadCount);

	// Writes out quads for the given voxel definition with the given origin offset in world space.
	// Returns number of quads written.
	int getQuads(const VoxelDefinition &voxelDef, const Int3 &voxel, double ceilingHeight,
		Quad *outQuads, int bufferSize);
	int getQuads(const VoxelDefinition &voxelDef, double ceilingHeight, Quad *outQuads, int bufferSize);
}

#endif
