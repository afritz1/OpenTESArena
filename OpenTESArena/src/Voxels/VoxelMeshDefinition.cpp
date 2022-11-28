#include <array>

#include "VoxelMeshDefinition.h"
#include "VoxelFacing2D.h"
#include "../World/MeshUtils.h"

#include "components/debug/Debug.h"

VoxelMeshDefinition::VoxelMeshDefinition()
{
	// Default to air voxel.
	this->uniqueVertexCount = 0;
	this->rendererVertexCount = 0;
	this->opaqueIndicesListCount = 0;
	this->alphaTestedIndicesListCount = 0;
	this->allowsBackFaces = false;
	this->enablesNeighborGeometry = false;
	this->isContextSensitive = false;
}

void VoxelMeshDefinition::initClassic(ArenaTypes::VoxelType voxelType, const ArenaMeshUtils::InitCache &meshInitCache)
{
	this->uniqueVertexCount = ArenaMeshUtils::GetUniqueVertexCount(voxelType);
	this->rendererVertexCount = ArenaMeshUtils::GetRendererVertexCount(voxelType);
	this->opaqueIndicesListCount = ArenaMeshUtils::GetOpaqueIndexBufferCount(voxelType);
	this->alphaTestedIndicesListCount = ArenaMeshUtils::GetAlphaTestedIndexBufferCount(voxelType);
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);

	if (voxelType != ArenaTypes::VoxelType::None)
	{
		const int rendererVertexPositionComponentCount = ArenaMeshUtils::GetRendererVertexPositionComponentCount(voxelType);
		this->rendererVertices.resize(rendererVertexPositionComponentCount);
		std::copy(meshInitCache.vertices.begin(), meshInitCache.vertices.begin() + rendererVertexPositionComponentCount, this->rendererVertices.data());

		const int rendererVertexNormalComponentCount = ArenaMeshUtils::GetRendererVertexNormalComponentCount(voxelType);
		this->rendererNormals.resize(rendererVertexNormalComponentCount);
		std::copy(meshInitCache.normals.begin(), meshInitCache.normals.begin() + rendererVertexNormalComponentCount, this->rendererNormals.data());

		const int rendererVertexTexCoordCount = ArenaMeshUtils::GetRendererVertexTexCoordCount(voxelType);
		this->rendererTexCoords.resize(rendererVertexTexCoordCount);
		std::copy(meshInitCache.texCoords.begin(), meshInitCache.texCoords.begin() + rendererVertexTexCoordCount, this->rendererTexCoords.data());
		
		for (int i = 0; i < this->opaqueIndicesListCount; i++)
		{
			std::vector<int32_t> &dstBuffer = this->getOpaqueIndicesList(i);
			const int opaqueIndexCount = ArenaMeshUtils::GetOpaqueIndexCount(voxelType, i);
			dstBuffer.resize(opaqueIndexCount);
			const BufferView<int32_t> &srcBuffer = (i == 0) ? meshInitCache.opaqueIndices0View :
				((i == 1) ? meshInitCache.opaqueIndices1View : meshInitCache.opaqueIndices2View);
			std::copy(srcBuffer.get(), srcBuffer.get() + opaqueIndexCount, dstBuffer.data());
		}

		if (this->alphaTestedIndicesListCount > 0)
		{
			const int alphaTestedIndexCount = ArenaMeshUtils::GetAlphaTestedIndexCount(voxelType, 0);
			this->alphaTestedIndices.resize(alphaTestedIndexCount);
			std::copy(meshInitCache.alphaTestedIndices0.begin(), meshInitCache.alphaTestedIndices0.begin() + alphaTestedIndexCount, this->alphaTestedIndices.data());
		}
	}
}

bool VoxelMeshDefinition::isEmpty() const
{
	return this->uniqueVertexCount == 0;
}

std::vector<int32_t> &VoxelMeshDefinition::getOpaqueIndicesList(int index)
{
	const std::array<std::vector<int32_t>*, 3> ptrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

const std::vector<int32_t> &VoxelMeshDefinition::getOpaqueIndicesList(int index) const
{
	const std::array<const std::vector<int32_t>*, 3> ptrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

void VoxelMeshDefinition::writeRendererGeometryBuffers(double ceilingScale, BufferView<double> outVertices,
	BufferView<double> outNormals, BufferView<double> outTexCoords) const
{
	static_assert(MeshUtils::POSITION_COMPONENTS_PER_VERTEX == 3);
	static_assert(MeshUtils::NORMAL_COMPONENTS_PER_VERTEX == 3);
	static_assert(MeshUtils::TEX_COORDS_PER_VERTEX == 2);
	DebugAssert(outVertices.getCount() >= this->rendererVertices.size());
	DebugAssert(outNormals.getCount() >= this->rendererNormals.size());
	DebugAssert(outTexCoords.getCount() >= this->rendererTexCoords.size());

	for (int i = 0; i < this->rendererVertexCount; i++)
	{
		const int index = i * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		const double srcX = this->rendererVertices[index];
		const double srcY = this->rendererVertices[index + 1];
		const double srcZ = this->rendererVertices[index + 2];
		const double dstX = srcX;
		const double dstY = srcY * ceilingScale;
		const double dstZ = srcZ;
		outVertices.set(index, dstX);
		outVertices.set(index + 1, dstY);
		outVertices.set(index + 2, dstZ);
	}

	std::copy(this->rendererNormals.begin(), this->rendererNormals.end(), outNormals.get());
	std::copy(this->rendererTexCoords.begin(), this->rendererTexCoords.end(), outTexCoords.get());
}

void VoxelMeshDefinition::writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0,
	BufferView<int32_t> outOpaqueIndices1, BufferView<int32_t> outOpaqueIndices2,
	BufferView<int32_t> outAlphaTestedIndices) const
{
	if (!this->opaqueIndices0.empty())
	{
		std::copy(this->opaqueIndices0.begin(), this->opaqueIndices0.end(), outOpaqueIndices0.get());
	}

	if (!this->opaqueIndices1.empty())
	{
		std::copy(this->opaqueIndices1.begin(), this->opaqueIndices1.end(), outOpaqueIndices1.get());
	}

	if (!this->opaqueIndices2.empty())
	{
		std::copy(this->opaqueIndices2.begin(), this->opaqueIndices2.end(), outOpaqueIndices2.get());
	}

	if (!this->alphaTestedIndices.empty())
	{
		std::copy(this->alphaTestedIndices.begin(), this->alphaTestedIndices.end(), outAlphaTestedIndices.get());
	}
}
