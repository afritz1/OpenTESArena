#ifndef VOXEL_GEOMETRY_H
#define VOXEL_GEOMETRY_H

#include <vector>

#include "LevelData.h"
#include "VoxelDefinition.h"
#include "../Math/Quad.h"
#include "../Math/Vector3.h"

// Mostly intended for collision geometry, not rendering.

namespace VoxelGeometry
{
	// Maximum possible quads for one voxel (intended for constexpr array initialization).
	static constexpr int MAX_QUADS = 6;

	// Returns number of quads for the given voxel definition and instance info.
	void getInfo(const VoxelDefinition &voxelDef, const Int3 &voxel,
		const LevelData::ChasmStates &chasmStates, int *outQuadCount);

	// Writes out quads for the given voxel definition, instance info, and origin offset
	// in world space. Returns number of quads written.
	int getQuads(const VoxelDefinition &voxelDef, const Int3 &voxel, double ceilingHeight,
		const LevelData::ChasmStates &chasmStates, Quad *outQuads, int bufferSize);
}

#endif
