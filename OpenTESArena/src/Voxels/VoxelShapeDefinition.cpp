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
	this->indicesListCount = 0;
	this->facingsListCount = 0;
}

void VoxelMeshDefinition::initClassic(ArenaVoxelType voxelType, VoxelShapeScaleType scaleType,
	const ArenaMeshUtils::ShapeInitCache &shapeInitCache)
{
	this->uniqueVertexCount = ArenaMeshUtils::GetUniqueVertexCount(voxelType);
	this->rendererVertexCount = ArenaMeshUtils::GetRendererVertexCount(voxelType);
	this->indicesListCount = ArenaMeshUtils::GetIndexBufferCount(voxelType);
	this->facingsListCount = ArenaMeshUtils::GetFacingBufferCount(voxelType);

	if (voxelType != ArenaVoxelType::None)
	{
		const int rendererVertexPositionComponentCount = ArenaMeshUtils::GetRendererVertexPositionComponentCount(voxelType);
		this->rendererPositions.resize(rendererVertexPositionComponentCount);
		std::copy(shapeInitCache.positions.begin(), shapeInitCache.positions.begin() + rendererVertexPositionComponentCount, this->rendererPositions.data());

		const int rendererVertexNormalComponentCount = ArenaMeshUtils::GetRendererVertexNormalComponentCount(voxelType);
		this->rendererNormals.resize(rendererVertexNormalComponentCount);
		std::copy(shapeInitCache.normals.begin(), shapeInitCache.normals.begin() + rendererVertexNormalComponentCount, this->rendererNormals.data());

		const int rendererVertexTexCoordComponentCount = ArenaMeshUtils::GetRendererVertexTexCoordComponentCount(voxelType);
		this->rendererTexCoords.resize(rendererVertexTexCoordComponentCount);
		std::copy(shapeInitCache.texCoords.begin(), shapeInitCache.texCoords.begin() + rendererVertexTexCoordComponentCount, this->rendererTexCoords.data());

		for (int i = 0; i < this->indicesListCount; i++)
		{
			std::vector<int32_t> &dstBuffer = this->getIndicesList(i);

			const int indexCount = ArenaMeshUtils::GetIndexBufferIndexCount(voxelType, i);
			dstBuffer.resize(indexCount);

			const BufferView<int32_t> *srcBuffer = nullptr;
			if (i == 0)
			{
				srcBuffer = &shapeInitCache.indices0View;
			}
			else if (i == 1)
			{
				srcBuffer = &shapeInitCache.indices1View;
			}
			else if (i == 2)
			{
				srcBuffer = &shapeInitCache.indices2View;
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(i));
			}

			std::copy(srcBuffer->begin(), srcBuffer->begin() + indexCount, dstBuffer.data());
		}

		for (int i = 0; i < this->facingsListCount; i++)
		{
			std::vector<VoxelFacing3D> &dstBuffer = this->getFacingsList(i);

			const int faceCount = ArenaMeshUtils::GetFacingBufferFaceCount(voxelType, i);
			dstBuffer.resize(faceCount);

			const BufferView<VoxelFacing3D> *srcBuffer = nullptr;
			if (i == 0)
			{
				srcBuffer = &shapeInitCache.facings0View;
			}
			else if (i == 1)
			{
				srcBuffer = &shapeInitCache.facings1View;
			}
			else if (i == 2)
			{
				srcBuffer = &shapeInitCache.facings2View;
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(i));
			}

			std::copy(srcBuffer->begin(), srcBuffer->begin() + faceCount, dstBuffer.data());
		}
	}
}

bool VoxelMeshDefinition::isEmpty() const
{
	return this->uniqueVertexCount == 0;
}

std::vector<int32_t> &VoxelMeshDefinition::getIndicesList(int index)
{
	std::vector<int32_t> *ptrs[] = { &this->indices0, &this->indices1, &this->indices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

BufferView<const int32_t> VoxelMeshDefinition::getIndicesList(int index) const
{
	const std::vector<int32_t> *ptrs[] = { &this->indices0, &this->indices1, &this->indices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

std::vector<VoxelFacing3D> &VoxelMeshDefinition::getFacingsList(int index)
{
	std::vector<VoxelFacing3D> *ptrs[] = { &this->facings0, &this->facings1, &this->facings2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

BufferView<const VoxelFacing3D> VoxelMeshDefinition::getFacingsList(int index) const
{
	const std::vector<VoxelFacing3D> *ptrs[] = { &this->facings0, &this->facings1, &this->facings2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

bool VoxelMeshDefinition::hasFullCoverageOfFacing(VoxelFacing3D facing) const
{
	// @todo eventually this should analyze the mesh + indices, using vertex position checks w/ epsilons

	for (int i = 0; i < this->facingsListCount; i++)
	{
		BufferView<const VoxelFacing3D> facingList = this->getFacingsList(i);
		for (const VoxelFacing3D currentFacing : facingList)
		{
			if (currentFacing == facing)
			{
				return true;
			}
		}
	}

	return false;
}

void VoxelMeshDefinition::writeRendererGeometryBuffers(VoxelShapeScaleType scaleType, double ceilingScale, BufferView<double> outPositions,
	BufferView<double> outNormals, BufferView<double> outTexCoords) const
{
	static_assert(MeshUtils::POSITION_COMPONENTS_PER_VERTEX == 3);
	static_assert(MeshUtils::NORMAL_COMPONENTS_PER_VERTEX == 3);
	static_assert(MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX == 2);
	DebugAssert(outPositions.getCount() >= this->rendererPositions.size());
	DebugAssert(outNormals.getCount() >= this->rendererNormals.size());
	DebugAssert(outTexCoords.getCount() >= this->rendererTexCoords.size());

	for (int i = 0; i < this->rendererVertexCount; i++)
	{
		const int index = i * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		const double srcX = this->rendererPositions[index];
		const double srcY = this->rendererPositions[index + 1];
		const double srcZ = this->rendererPositions[index + 2];
		const double dstX = srcX;
		const double dstY = MeshUtils::getScaledVertexY(srcY, scaleType, ceilingScale);
		const double dstZ = srcZ;
		outPositions.set(index, dstX);
		outPositions.set(index + 1, dstY);
		outPositions.set(index + 2, dstZ);
	}

	std::copy(this->rendererNormals.begin(), this->rendererNormals.end(), outNormals.begin());
	std::copy(this->rendererTexCoords.begin(), this->rendererTexCoords.end(), outTexCoords.begin());
}

void VoxelMeshDefinition::writeRendererIndexBuffers(BufferView<int32_t> outIndices0, BufferView<int32_t> outIndices1,
	BufferView<int32_t> outIndices2) const
{
	if (!this->indices0.empty())
	{
		std::copy(this->indices0.begin(), this->indices0.end(), outIndices0.begin());
	}

	if (!this->indices1.empty())
	{
		std::copy(this->indices1.begin(), this->indices1.end(), outIndices1.begin());
	}

	if (!this->indices2.empty())
	{
		std::copy(this->indices2.begin(), this->indices2.end(), outIndices2.begin());
	}
}

VoxelShapeDefinition::VoxelShapeDefinition()
{
	// Air by default. Needs the shape defined in case of trigger voxels, but doesn't need a render mesh.
	ArenaMeshUtils::ShapeInitCache airShapeInitCache;
	airShapeInitCache.initDefaultBoxValues();
	this->initBoxFromClassic(ArenaVoxelType::None, VoxelShapeScaleType::ScaledFromMin, airShapeInitCache);
}

void VoxelShapeDefinition::initBoxFromClassic(ArenaVoxelType voxelType, VoxelShapeScaleType scaleType, const ArenaMeshUtils::ShapeInitCache &shapeInitCache)
{
	this->type = VoxelShapeType::Box;
	this->box.init(shapeInitCache.boxWidth, shapeInitCache.boxHeight, shapeInitCache.boxDepth, shapeInitCache.boxYOffset, shapeInitCache.boxYRotation);
	this->mesh.initClassic(voxelType, scaleType, shapeInitCache);
	this->scaleType = scaleType;
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->allowsAdjacentDoorFaces = ArenaMeshUtils::AllowsAdjacentDoorFaces(voxelType);
	this->allowsInternalFaceRemoval = ArenaMeshUtils::AllowsInternalFaceRemoval(voxelType);
	this->allowsAdjacentFaceCombining = ArenaMeshUtils::AllowsAdjacentFaceCombining(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);
	this->isElevatedPlatform = ArenaMeshUtils::IsElevatedPlatform(voxelType);
}
