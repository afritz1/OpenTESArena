#ifndef MESH_UTILS_H
#define MESH_UTILS_H

enum class VoxelShapeScaleType;

namespace MeshUtils
{
	static constexpr int INDICES_PER_TRIANGLE = 3;
	static constexpr int INDICES_PER_FACE = INDICES_PER_TRIANGLE * 2; // Two triangles per quad
	static constexpr int POSITION_COMPONENTS_PER_VERTEX = 3; // XYZ position
	static constexpr int NORMAL_COMPONENTS_PER_VERTEX = 3; // XYZ direction
	static constexpr int TEX_COORDS_PER_VERTEX = 2; // UV texture coordinates

	// For positioning raised platforms, etc. correctly.
	double getScaledVertexY(double meshY, VoxelShapeScaleType scaleType, double ceilingScale);
}

#endif
