#ifndef VOXEL_MESH_DEFINITION_H
#define VOXEL_MESH_DEFINITION_H

#include <vector> // @todo: Buffer<T> would be better although there were annoyances with the deleted copy constructor

#include "../Assets/ArenaTypes.h"
#include "../World/ArenaMeshUtils.h"

#include "components/utilities/BufferView.h"

// Intended to be stored per level rather than shared across levels due to how some voxel types
// like raised voxels have unique offset and size requirements.

struct VoxelMeshDefinition
{
	std::vector<double> rendererVertices;
	std::vector<double> rendererNormals;
	std::vector<double> rendererTexCoords;
	std::vector<int32_t> opaqueIndices0, opaqueIndices1, opaqueIndices2, alphaTestedIndices;
	int uniqueVertexCount; // Ideal number of vertices to represent the mesh.
	int rendererVertexCount; // Number of vertices required by rendering due to vertex attributes.
	int opaqueIndicesListCount, alphaTestedIndicesListCount;
	bool allowsBackFaces;
	bool enablesNeighborGeometry; // For voxels that influence adjacent context-sensitive voxels like chasms.
	bool isContextSensitive; // For voxels like chasms whose geometry is conditional to what's around them.

	VoxelMeshDefinition();

	void initClassic(ArenaTypes::VoxelType voxelType, const ArenaMeshUtils::InitCache &meshInitCache);

	bool isEmpty() const;
	std::vector<int32_t> &getOpaqueIndicesList(int index);
	const std::vector<int32_t> &getOpaqueIndicesList(int index) const;

	void writeRendererGeometryBuffers(double ceilingScale, BufferView<double> outVertices, BufferView<double> outNormals,
		BufferView<double> outTexCoords) const;
	void writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0, BufferView<int32_t> outOpaqueIndices1,
		BufferView<int32_t> outOpaqueIndices2, BufferView<int32_t> outAlphaTestedIndices) const;
};

#endif
