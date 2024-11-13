#include <cstring>

#include "ArenaChasmUtils.h"
#include "VoxelShapeDefinition.h"
#include "../Collision/Physics.h"

#include "components/debug/Debug.h"

void VoxelBoxShapeDefinition::init(double width, double height, double depth, double yOffset, Radians yRotation)
{
	DebugAssert(width > 0.0);
	DebugAssert(height > 0.0);
	DebugAssert(depth > 0.0);
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->yOffset = yOffset;
	this->yRotation = yRotation;
}

VoxelMeshDefinition::VoxelMeshDefinition()
{
	// Default to air voxel.
	this->uniqueVertexCount = 0;
	this->rendererVertexCount = 0;
	this->opaqueIndicesListCount = 0;
	this->alphaTestedIndicesListCount = 0;
}

void VoxelMeshDefinition::initClassic(ArenaTypes::VoxelType voxelType, VoxelShapeScaleType scaleType,
	const ArenaMeshUtils::ShapeInitCache &shapeInitCache)
{
	this->uniqueVertexCount = ArenaMeshUtils::GetUniqueVertexCount(voxelType);
	this->rendererVertexCount = ArenaMeshUtils::GetRendererVertexCount(voxelType);
	this->opaqueIndicesListCount = ArenaMeshUtils::GetOpaqueIndexBufferCount(voxelType);
	this->alphaTestedIndicesListCount = ArenaMeshUtils::GetAlphaTestedIndexBufferCount(voxelType);

	if (voxelType != ArenaTypes::VoxelType::None)
	{
		const int rendererVertexPositionComponentCount = ArenaMeshUtils::GetRendererVertexPositionComponentCount(voxelType);
		this->rendererVertices.resize(rendererVertexPositionComponentCount);
		std::copy(shapeInitCache.vertices.begin(), shapeInitCache.vertices.begin() + rendererVertexPositionComponentCount, this->rendererVertices.data());

		const int rendererVertexNormalComponentCount = ArenaMeshUtils::GetRendererVertexNormalComponentCount(voxelType);
		this->rendererNormals.resize(rendererVertexNormalComponentCount);
		std::copy(shapeInitCache.normals.begin(), shapeInitCache.normals.begin() + rendererVertexNormalComponentCount, this->rendererNormals.data());

		const int rendererVertexTexCoordComponentCount = ArenaMeshUtils::GetRendererVertexTexCoordComponentCount(voxelType);
		this->rendererTexCoords.resize(rendererVertexTexCoordComponentCount);
		std::copy(shapeInitCache.texCoords.begin(), shapeInitCache.texCoords.begin() + rendererVertexTexCoordComponentCount, this->rendererTexCoords.data());

		for (int i = 0; i < this->opaqueIndicesListCount; i++)
		{
			std::vector<int32_t> &dstBuffer = this->getOpaqueIndicesList(i);
			const int opaqueIndexCount = ArenaMeshUtils::GetOpaqueIndexCount(voxelType, i);
			dstBuffer.resize(opaqueIndexCount);
			const BufferView<int32_t> &srcBuffer = (i == 0) ? shapeInitCache.opaqueIndices0View :
				((i == 1) ? shapeInitCache.opaqueIndices1View : shapeInitCache.opaqueIndices2View);
			std::copy(srcBuffer.begin(), srcBuffer.begin() + opaqueIndexCount, dstBuffer.data());
		}

		if (this->alphaTestedIndicesListCount > 0)
		{
			const int alphaTestedIndexCount = ArenaMeshUtils::GetAlphaTestedIndexCount(voxelType, 0);
			this->alphaTestedIndices.resize(alphaTestedIndexCount);
			std::copy(shapeInitCache.alphaTestedIndices0.begin(), shapeInitCache.alphaTestedIndices0.begin() + alphaTestedIndexCount, this->alphaTestedIndices.data());
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

BufferView<const int32_t> VoxelMeshDefinition::getOpaqueIndicesList(int index) const
{
	const std::array<const std::vector<int32_t>*, 3> ptrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

void VoxelMeshDefinition::writeRendererGeometryBuffers(VoxelShapeScaleType scaleType, double ceilingScale, BufferView<double> outVertices,
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
		const double dstY = MeshUtils::getScaledVertexY(srcY, scaleType, ceilingScale);
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

VoxelShapeDefinition::VoxelShapeDefinition()
{
	// Air by default.
	this->initNone();
}

void VoxelShapeDefinition::initNone()
{
	this->type = VoxelShapeType::None;
	std::memset(&this->box, 0, sizeof(this->box));

	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::None;
	constexpr VoxelShapeScaleType scaleType = VoxelShapeScaleType::ScaledFromMin;
	ArenaMeshUtils::ShapeInitCache shapeInitCache;
	this->mesh.initClassic(voxelType, scaleType, shapeInitCache);
	this->scaleType = scaleType;
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->allowsAdjacentDoorFaces = ArenaMeshUtils::AllowsAdjacentDoorFaces(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);
}

void VoxelShapeDefinition::initBoxFromClassic(ArenaTypes::VoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaMeshUtils::ShapeInitCache &shapeInitCache)
{
	this->type = VoxelShapeType::Box;
	this->box.init(shapeInitCache.boxWidth, shapeInitCache.boxHeight, shapeInitCache.boxDepth, shapeInitCache.boxYOffset, shapeInitCache.boxYRotation);
	this->mesh.initClassic(voxelType, scaleType, shapeInitCache);
	this->scaleType = scaleType;
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->allowsAdjacentDoorFaces = ArenaMeshUtils::AllowsAdjacentDoorFaces(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);
}