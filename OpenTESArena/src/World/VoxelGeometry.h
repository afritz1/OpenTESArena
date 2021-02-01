#ifndef VOXEL_GEOMETRY_H
#define VOXEL_GEOMETRY_H

#include "VoxelUtils.h"

// Mostly intended for collision geometry, not rendering.

class Quad;
class VoxelDefinition;
class VoxelInstance;

namespace VoxelGeometry
{
	// Maximum possible quads for one voxel (intended for constexpr array initialization).
	static constexpr int MAX_QUADS = 6;

	// Returns number of quads for the given voxel definition and instance info.
	void getInfo(const VoxelDefinition &voxelDef, const VoxelInstance *voxelInst, int *outQuadCount);

	// Writes out quads for the given voxel definition, instance info, and origin offset
	// in world space. Returns number of quads written.
	int getQuads(const VoxelDefinition &voxelDef, const NewInt3 &voxel, double ceilingHeight,
		const VoxelInstance *voxelInst, Quad *outQuads, int bufferSize);
}

#endif
