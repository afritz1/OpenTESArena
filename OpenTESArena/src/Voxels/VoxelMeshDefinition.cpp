#include <array>

#include "ArenaChasmUtils.h"
#include "VoxelMeshDefinition.h"
#include "VoxelFacing2D.h"
#include "../World/MeshUtils.h"

#include "components/debug/Debug.h"

VoxelMeshDefinition::VoxelMeshDefinition()
{
	// Default to air voxel.
	this->uniqueVertexCount = 0;
	this->rendererVertexCount = 0;
	this->collisionVertexCount = 0;
	this->opaqueIndicesListCount = 0;
	this->alphaTestedIndicesListCount = 0;
	this->scaleType = VoxelMeshScaleType::ScaledFromMin;
	this->allowsBackFaces = false;
	this->enablesNeighborGeometry = false;
	this->isContextSensitive = false;
}

void VoxelMeshDefinition::initClassic(ArenaTypes::VoxelType voxelType, VoxelMeshScaleType scaleType,
	const ArenaMeshUtils::RenderMeshInitCache &renderMeshInitCache,
	const ArenaMeshUtils::CollisionMeshInitCache &collisionMeshInitCache)
{
	this->uniqueVertexCount = ArenaMeshUtils::GetUniqueVertexCount(voxelType);
	this->rendererVertexCount = ArenaMeshUtils::GetRendererVertexCount(voxelType);
	this->collisionVertexCount = this->uniqueVertexCount;
	this->opaqueIndicesListCount = ArenaMeshUtils::GetOpaqueIndexBufferCount(voxelType);
	this->alphaTestedIndicesListCount = ArenaMeshUtils::GetAlphaTestedIndexBufferCount(voxelType);
	this->scaleType = scaleType;
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);

	if (voxelType != ArenaTypes::VoxelType::None)
	{
		const int rendererVertexPositionComponentCount = ArenaMeshUtils::GetRendererVertexPositionComponentCount(voxelType);
		this->rendererVertices.resize(rendererVertexPositionComponentCount);
		std::copy(renderMeshInitCache.vertices.begin(), renderMeshInitCache.vertices.begin() + rendererVertexPositionComponentCount, this->rendererVertices.data());

		const int rendererVertexNormalComponentCount = ArenaMeshUtils::GetRendererVertexNormalComponentCount(voxelType);
		this->rendererNormals.resize(rendererVertexNormalComponentCount);
		std::copy(renderMeshInitCache.normals.begin(), renderMeshInitCache.normals.begin() + rendererVertexNormalComponentCount, this->rendererNormals.data());

		const int rendererVertexTexCoordComponentCount = ArenaMeshUtils::GetRendererVertexTexCoordComponentCount(voxelType);
		this->rendererTexCoords.resize(rendererVertexTexCoordComponentCount);
		std::copy(renderMeshInitCache.texCoords.begin(), renderMeshInitCache.texCoords.begin() + rendererVertexTexCoordComponentCount, this->rendererTexCoords.data());

		const int collisionVertexPositionComponentCount = ArenaMeshUtils::GetCollisionVertexPositionComponentCount(voxelType);
		this->collisionVertices.resize(collisionVertexPositionComponentCount);
		std::copy(collisionMeshInitCache.vertices.begin(), collisionMeshInitCache.vertices.begin() + collisionVertexPositionComponentCount, this->collisionVertices.data());

		const int collisionFaceNormalComponentCount = ArenaMeshUtils::GetCollisionFaceNormalComponentCount(voxelType);
		this->collisionNormals.resize(collisionFaceNormalComponentCount);
		std::copy(collisionMeshInitCache.normals.begin(), collisionMeshInitCache.normals.begin() + collisionFaceNormalComponentCount, this->collisionNormals.data());
		
		for (int i = 0; i < this->opaqueIndicesListCount; i++)
		{
			std::vector<int32_t> &dstBuffer = this->getOpaqueIndicesList(i);
			const int opaqueIndexCount = ArenaMeshUtils::GetOpaqueIndexCount(voxelType, i);
			dstBuffer.resize(opaqueIndexCount);
			const BufferView<int32_t> &srcBuffer = (i == 0) ? renderMeshInitCache.opaqueIndices0View :
				((i == 1) ? renderMeshInitCache.opaqueIndices1View : renderMeshInitCache.opaqueIndices2View);
			std::copy(srcBuffer.begin(), srcBuffer.begin() + opaqueIndexCount, dstBuffer.data());
		}

		if (this->alphaTestedIndicesListCount > 0)
		{
			const int alphaTestedIndexCount = ArenaMeshUtils::GetAlphaTestedIndexCount(voxelType, 0);
			this->alphaTestedIndices.resize(alphaTestedIndexCount);
			std::copy(renderMeshInitCache.alphaTestedIndices0.begin(), renderMeshInitCache.alphaTestedIndices0.begin() + alphaTestedIndexCount, this->alphaTestedIndices.data());
		}

		const int collisionIndexCount = ArenaMeshUtils::GetCollisionIndexCount(voxelType);
		this->collisionIndices.resize(collisionIndexCount);
		std::copy(collisionMeshInitCache.indices.begin(), collisionMeshInitCache.indices.begin() + collisionIndexCount, this->collisionIndices.data());
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
		const double dstY = MeshUtils::getScaledVertexY(srcY, this->scaleType, ceilingScale);
		const double dstZ = srcZ;
		outVertices.set(index, dstX);
		outVertices.set(index + 1, dstY);
		outVertices.set(index + 2, dstZ);
	}

	std::copy(this->rendererNormals.begin(), this->rendererNormals.end(), outNormals.begin());
	std::copy(this->rendererTexCoords.begin(), this->rendererTexCoords.end(), outTexCoords.begin());
}

void VoxelMeshDefinition::writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0,
	BufferView<int32_t> outOpaqueIndices1, BufferView<int32_t> outOpaqueIndices2,
	BufferView<int32_t> outAlphaTestedIndices) const
{
	if (!this->opaqueIndices0.empty())
	{
		std::copy(this->opaqueIndices0.begin(), this->opaqueIndices0.end(), outOpaqueIndices0.begin());
	}

	if (!this->opaqueIndices1.empty())
	{
		std::copy(this->opaqueIndices1.begin(), this->opaqueIndices1.end(), outOpaqueIndices1.begin());
	}

	if (!this->opaqueIndices2.empty())
	{
		std::copy(this->opaqueIndices2.begin(), this->opaqueIndices2.end(), outOpaqueIndices2.begin());
	}

	if (!this->alphaTestedIndices.empty())
	{
		std::copy(this->alphaTestedIndices.begin(), this->alphaTestedIndices.end(), outAlphaTestedIndices.begin());
	}
}
