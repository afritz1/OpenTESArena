#ifndef VOXEL_MESH_DEFINITION_H
#define VOXEL_MESH_DEFINITION_H

#include <vector> // @todo: Buffer<T> would be better although there were annoyances with the deleted copy constructor

#include "../Assets/ArenaTypes.h"
#include "../World/ArenaMeshUtils.h"

#include "components/utilities/BufferView.h"

// For voxels that are affected differently by ceiling scale (e.g. raised platforms and water/lava chasms).
enum class VoxelMeshScaleType
{
	ScaledFromMin,
	UnscaledFromMin,
	UnscaledFromMax
};

// Intended to be stored per level rather than shared across levels due to how some voxel types
// like raised voxels have unique offset and size requirements.
struct VoxelMeshDefinition
{
	std::vector<double> rendererVertices, rendererNormals, rendererTexCoords;
	std::vector<double> collisionVertices, collisionNormals;
	std::vector<int32_t> opaqueIndices0, opaqueIndices1, opaqueIndices2, alphaTestedIndices;
	int uniqueVertexCount; // Ideal number of vertices to represent the mesh.
	int rendererVertexCount; // Number of vertices required by rendering due to vertex attributes.
	int collisionVertexCount;
	int opaqueIndicesListCount, alphaTestedIndicesListCount;
	VoxelMeshScaleType scaleType;
	bool allowsBackFaces;
	bool enablesNeighborGeometry; // For voxels that influence adjacent context-sensitive voxels like chasms.
	bool isContextSensitive; // For voxels like chasms whose geometry is conditional to what's around them.

	VoxelMeshDefinition();

	void initClassic(ArenaTypes::VoxelType voxelType, VoxelMeshScaleType scaleType,
		const ArenaMeshUtils::RenderMeshInitCache &renderMeshInitCache,
		const ArenaMeshUtils::CollisionMeshInitCache &collisionMeshInitCache);

	bool isEmpty() const;
	std::vector<int32_t> &getOpaqueIndicesList(int index);
	const std::vector<int32_t> &getOpaqueIndicesList(int index) const;

	void writeRendererGeometryBuffers(double ceilingScale, BufferView<double> outVertices,
		BufferView<double> outNormals, BufferView<double> outTexCoords) const;
	void writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0, BufferView<int32_t> outOpaqueIndices1,
		BufferView<int32_t> outOpaqueIndices2, BufferView<int32_t> outAlphaTestedIndices) const;
};

#endif
