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
	std::vector<double> rendererPositions, rendererNormals, rendererTexCoords;
	std::vector<int32_t> indices0, indices1, indices2; // Up to 3 draw calls.
	std::vector<VoxelFacing3D> facings0, facings1, facings2; // Up to 3 sets of fully-covered voxel faces, associated with index buffers, used with face combining.
	int uniqueVertexCount; // Ideal number of vertices to represent the mesh.
	int rendererVertexCount; // Number of vertices required by rendering due to vertex attributes.
	int indicesListCount;
	int facingsListCount;

	VoxelMeshDefinition();

	void initClassic(ArenaVoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaShapeInitCache &shapeInitCache);

	bool isEmpty() const;
	std::vector<int32_t> &getIndicesList(int index);
	BufferView<const int32_t> getIndicesList(int index) const;
	std::vector<VoxelFacing3D> &getFacingsList(int index);
	BufferView<const VoxelFacing3D> getFacingsList(int index) const;

	// Finds the index buffer (if any) that fully covers the voxel facing. Used with mesh combining.
	int findIndexBufferIndexWithFacing(VoxelFacing3D facing) const;
	bool hasFullCoverageOfFacing(VoxelFacing3D facing) const;

	void writeRendererVertexPositionBuffer(VoxelShapeScaleType scaleType, double ceilingScale, BufferView<double> outPositions) const;
	void writeRendererVertexNormalBuffer(BufferView<double> outNormals) const;
	void writeRendererVertexTexCoordBuffer(BufferView<double> outTexCoords) const;
	void writeRendererIndexBuffers(BufferView<int32_t> outIndices0, BufferView<int32_t> outIndices1, BufferView<int32_t> outIndices2) const;
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
	bool allowsBackFaces; // Back face culling for rendering.
	bool allowsAdjacentDoorFaces; // For voxels that don't prevent a door's face from rendering.
	bool allowsInternalFaceRemoval; // For voxels that can disable their faces when blocked by an opaque neighbor's face.
	bool allowsAdjacentFaceCombining; // For voxels that can combine their faces with adjacent voxel faces to create a larger mesh.
	bool enablesNeighborGeometry; // For voxels that influence adjacent context-sensitive voxels like chasms.
	bool isContextSensitive; // For voxels like chasms whose geometry is conditional to what's around them.
	bool isElevatedPlatform; // For voxels that entities sit on top of and for letting player sleep in peace.

	VoxelShapeDefinition();

	void initBoxFromClassic(ArenaVoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaShapeInitCache &shapeInitCache);
};

#endif
