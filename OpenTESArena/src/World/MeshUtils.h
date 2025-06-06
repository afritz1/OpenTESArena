#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <cstdint>

#include "Coord.h"
#include "../Math/Vector3.h"

#include "components/utilities/BufferView.h"

enum class VoxelFacing3D;
enum class VoxelShapeScaleType;

// Various helper functions for planar vertex components. Not using interleaved since some vertices
// like for voxels may have extra values in the future.
namespace MeshUtils
{
	static constexpr int INDICES_PER_TRIANGLE = 3; // Example: [0, 1, 2].
	static constexpr int INDICES_PER_QUAD = INDICES_PER_TRIANGLE * 2; // Example: [0, 1, 2, 2, 3, 0].
	
	static constexpr int POSITION_COMPONENTS_PER_VERTEX = 3; // XYZ position.
	static constexpr int POSITION_COMPONENTS_PER_TRIANGLE = POSITION_COMPONENTS_PER_VERTEX * 3;

	static constexpr int NORMAL_COMPONENTS_PER_VERTEX = 3; // XYZ direction.
	static constexpr int NORMAL_COMPONENTS_PER_TRIANGLE = NORMAL_COMPONENTS_PER_VERTEX * 3;

	static constexpr int TEX_COORD_COMPONENTS_PER_VERTEX = 2; // UV texture coordinates.
	static constexpr int TEX_COORD_COMPONENTS_PER_TRIANGLE = TEX_COORD_COMPONENTS_PER_VERTEX * 3;

	// Returns the number of vertices these planar components completely fill. Does not handle leftovers.
	int getVertexCount(BufferView<const double> components, int componentsPerVertex);

	// Returns the number of triangles these planar components completely fill. Does not handle leftovers.
	int getTriangleCount(BufferView<const double> components, int componentsPerVertex);

	bool isEmpty(BufferView<const double> components);
	bool isFinite(BufferView<const double> components);

	// Whether this planar range defines something impossible like an empty mesh or one infinitely large.
	bool isValid(BufferView<const double> components);
	
	// Whether this planar range provides completely for triangles with no leftovers.
	bool isComplete(BufferView<const double> components, int componentsPerVertex);

	bool hasValidNormals(BufferView<const double> normals);

	// Whether these tex coords are within the max range for repeating, like [0, 4).
	bool hasValidTexCoords(BufferView<const double> uvs, double maxU, double maxV);

	Double3 getVertexPositionAtIndex(BufferView<const double> positions, int vertexIndex);
	Double3 getVertexNormalAtIndex(BufferView<const double> normals, int vertexIndex);
	Double2 getVertexTexCoordAtIndex(BufferView<const double> uvs, int vertexIndex);

	int findDuplicateVertexPosition(BufferView<const double> positions, double x, double y, double z, int startVertexIndex = 0);

	// Creates a normal facing out of the three vertices (nine components) starting at the given index.
	Double3 createVertexNormalAtIndex(BufferView<const double> positions, int vertexIndex);

	// Creates quad vertex positions/attributes/indices counterclockwise (top left - bottom left - bottom right - top right).
	// To get world space, translate model space vertices by the 'min' point.
	void createVoxelFaceQuadPositionsModelSpace(const VoxelInt3 &min, const VoxelInt3 &max, VoxelFacing3D facing, double ceilingScale, BufferView<Double3> outPositions);
	void createVoxelFaceQuadNormals(VoxelFacing3D facing, BufferView<Double3> outNormals);
	void createVoxelFaceQuadTexCoords(int width, int height, BufferView<Double2> outUVs);
	void createVoxelFaceQuadIndices(BufferView<int32_t> outIndices);

	// For positioning raised platforms, etc. correctly.
	double getScaledVertexY(double meshY, VoxelShapeScaleType scaleType, double ceilingScale);
}

#endif
