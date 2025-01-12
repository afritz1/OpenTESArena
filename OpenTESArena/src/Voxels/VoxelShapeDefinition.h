#ifndef VOXEL_SHAPE_DEFINITION_H
#define VOXEL_SHAPE_DEFINITION_H

#include <vector> // @todo: Buffer<T> would be better although there were annoyances with the deleted copy constructor

#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../World/ArenaMeshUtils.h"

#include "components/utilities/BufferView.h"

enum class VoxelShapeType
{
	Box
};

// For voxels that are affected differently by ceiling scale (e.g. raised platforms and water/lava chasms).
enum class VoxelShapeScaleType
{
	ScaledFromMin,
	UnscaledFromMin,
	UnscaledFromMax
};

struct VoxelBoxShapeDefinition
{
	double width, height, depth;
	double yOffset; // Elevation above bottom of voxel.
	Radians yRotation; // For diagonal walls.

	void init(double width, double height, double depth, double yOffset, Radians yRotation);
};

// For rendering.
struct VoxelMeshDefinition
{
	std::vector<double> rendererVertices, rendererNormals, rendererTexCoords;
	std::vector<int32_t> opaqueIndices0, opaqueIndices1, opaqueIndices2, alphaTestedIndices;
	int uniqueVertexCount; // Ideal number of vertices to represent the mesh.
	int rendererVertexCount; // Number of vertices required by rendering due to vertex attributes.
	int opaqueIndicesListCount, alphaTestedIndicesListCount;

	VoxelMeshDefinition();

	void initClassic(ArenaTypes::VoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaMeshUtils::ShapeInitCache &shapeInitCache);

	bool isEmpty() const;
	std::vector<int32_t> &getOpaqueIndicesList(int index);
	BufferView<const int32_t> getOpaqueIndicesList(int index) const;

	void writeRendererGeometryBuffers(VoxelShapeScaleType scaleType, double ceilingScale, BufferView<double> outVertices,
		BufferView<double> outNormals, BufferView<double> outTexCoords) const;
	void writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0, BufferView<int32_t> outOpaqueIndices1,
		BufferView<int32_t> outOpaqueIndices2, BufferView<int32_t> outAlphaTestedIndices) const;
};

// Provides geometry for physics and rendering.
struct VoxelShapeDefinition
{
	VoxelShapeType type;

	union
	{
		VoxelBoxShapeDefinition box;
	};

	VoxelMeshDefinition mesh;
	VoxelShapeScaleType scaleType;
	bool allowsBackFaces;
	bool allowsAdjacentDoorFaces; // For voxels that don't prevent a door's face from rendering.
	bool enablesNeighborGeometry; // For voxels that influence adjacent context-sensitive voxels like chasms.
	bool isContextSensitive; // For voxels like chasms whose geometry is conditional to what's around them.
	bool isElevatedPlatform; // For voxels that entities sit on top of and for letting player sleep in peace.

	VoxelShapeDefinition();

	void initBoxFromClassic(ArenaTypes::VoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaMeshUtils::ShapeInitCache &shapeInitCache);
};

#endif
