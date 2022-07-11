#ifndef VOXEL_MESH_DEFINITION_H
#define VOXEL_MESH_DEFINITION_H

#include <vector> // @todo: Buffer<T> would be better although there were annoyances with the deleted copy constructor

#include "components/utilities/BufferView.h"

// Intended to be stored per level rather than shared across levels due to how some voxel types
// like raised voxels have unique offset and size requirements.

class VoxelDefinition;

using VoxelMeshDefID = int;

struct VoxelMeshDefinition
{
	static constexpr int MAX_VERTICES = 24;
	static constexpr int MAX_INDICES = 36;
	static constexpr int INDICES_PER_TRIANGLE = 3;
	static constexpr int COMPONENTS_PER_VERTEX = 3; // XYZ
	static constexpr int ATTRIBUTES_PER_VERTEX = 2; // UV texture coordinates

	std::vector<double> rendererVertices;
	std::vector<double> rendererAttributes;
	std::vector<int32_t> opaqueIndices0, opaqueIndices1, opaqueIndices2, alphaTestedIndices;
	int uniqueVertexCount; // Ideal number of vertices to represent the mesh.
	int rendererVertexCount; // Number of vertices required by rendering due to vertex attributes.
	int opaqueIndicesListCount, alphaTestedIndicesListCount;
	bool allowsBackFaces;
	bool enablesNeighborGeometry; // For adjacent context-sensitive voxels like chasms.

	VoxelMeshDefinition();

	void initClassic(const VoxelDefinition &voxelDef);

	bool isEmpty() const;
	std::vector<int32_t> &getOpaqueIndicesList(int index);
	const std::vector<int32_t> &getOpaqueIndicesList(int index) const;

	void writeRendererGeometryBuffers(double ceilingScale, BufferView<double> outVertices, BufferView<double> outAttributes) const;
	void writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0, BufferView<int32_t> outOpaqueIndices1,
		BufferView<int32_t> outOpaqueIndices2, BufferView<int32_t> outAlphaTestedIndices) const;
};

#endif
