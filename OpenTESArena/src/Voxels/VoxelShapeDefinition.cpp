#include <algorithm>
#include <cstring>

#include "ArenaChasmUtils.h"
#include "VoxelShapeDefinition.h"
#include "../Collision/Physics.h"
#include "../World/MeshLibrary.h"

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
	this->indicesListCount = 0;
	this->facingCount = 0;
	this->textureSlotIndexCount = 0;
}

void VoxelMeshDefinition::initClassic(const ArenaShapeInitCache &shapeInitCache, VoxelShapeScaleType scaleType, double ceilingScale)
{
	const ArenaVoxelType voxelType = shapeInitCache.voxelType;
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(voxelType);

	this->indicesListCount = 0;
	this->facingCount = 0;
	this->textureSlotIndexCount = 0;

	int vertexCount = 0;
	for (int i = 0; i < meshEntries.getCount(); i++)
	{
		const MeshLibraryEntry &entry = meshEntries[i];
		if (voxelType == ArenaVoxelType::Edge)
		{
			const VoxelFacing3D edgeFacing3D = VoxelUtils::convertFaceTo3D(shapeInitCache.edge.facing);
			if (entry.facing != edgeFacing3D)
			{
				continue;
			}
		}

		Span<const int32_t> meshIndices = entry.vertexIndices;
		std::vector<int32_t> &indicesList = this->indicesLists[this->indicesListCount];
		indicesList.resize(meshIndices.getCount());
		std::transform(meshIndices.begin(), meshIndices.end(), indicesList.begin(),
			[&vertexCount](const int32_t meshIndex)
		{
			return meshIndex + vertexCount;
		});

		this->indicesListCount++;

		vertexCount += static_cast<int>(entry.vertices.size());

		if (entry.facing.has_value())
		{
			this->facings[this->facingCount] = *entry.facing;
			this->facingCount++;
		}

		this->textureSlotIndices[this->textureSlotIndexCount] = entry.textureSlotIndex;
		this->textureSlotIndexCount++;
	}

	const int rendererVertexPositionComponentCount = vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	const int rendererVertexNormalComponentCount = vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	const int rendererVertexTexCoordComponentCount = vertexCount * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	this->rendererPositions.resize(rendererVertexPositionComponentCount);
	this->rendererNormals.resize(rendererVertexNormalComponentCount);
	this->rendererTexCoords.resize(rendererVertexTexCoordComponentCount);

	switch (voxelType)
	{
	case ArenaVoxelType::None:
		break;
	case ArenaVoxelType::Wall:
	case ArenaVoxelType::Floor:
	case ArenaVoxelType::Ceiling:
	case ArenaVoxelType::TransparentWall:
	case ArenaVoxelType::Chasm:
	case ArenaVoxelType::Door:
	{
		int destinationVertexIndex = 0;
		for (const MeshLibraryEntry &meshEntry : meshEntries)
		{
			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
				this->rendererPositions[outPositionsIndex] = sourceVertex.positionX;
				this->rendererPositions[outPositionsIndex + 1] = sourceVertex.positionY; // @todo move getScaledVertexY from RenderVoxelChunkManager here
				this->rendererPositions[outPositionsIndex + 2] = sourceVertex.positionZ;
				this->rendererNormals[outNormalsIndex] = sourceVertex.normalX;
				this->rendererNormals[outNormalsIndex + 1] = sourceVertex.normalY;
				this->rendererNormals[outNormalsIndex + 2] = sourceVertex.normalZ;
				this->rendererTexCoords[outTexCoordsIndex] = sourceVertex.texCoordU;
				this->rendererTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;
				destinationVertexIndex++;
			}
		}

		break;
	}
	case ArenaVoxelType::Raised:
	{
		DebugAssert(shapeInitCache.voxelType == ArenaVoxelType::Raised);

		double yMax = std::numeric_limits<double>::lowest();
		double vMax = std::numeric_limits<double>::lowest();

		for (const MeshLibraryEntry &meshEntry : meshEntries)
		{
			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				yMax = std::max(yMax, sourceVertex.positionY);
				vMax = std::max(vMax, sourceVertex.texCoordV);
			}
		}

		int destinationVertexIndex = 0;
		for (const MeshLibraryEntry &meshEntry : meshEntries)
		{
			const bool isSideFace = (meshEntry.facing != VoxelFacing3D::PositiveY) && (meshEntry.facing != VoxelFacing3D::NegativeY);

			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;

				this->rendererPositions[outPositionsIndex] = sourceVertex.positionX;

				this->rendererPositions[outPositionsIndex + 1] = shapeInitCache.boxYOffset;
				if (sourceVertex.positionY == yMax)
				{
					this->rendererPositions[outPositionsIndex + 1] += shapeInitCache.boxHeight;
				}

				this->rendererPositions[outPositionsIndex + 2] = sourceVertex.positionZ;

				this->rendererNormals[outNormalsIndex] = sourceVertex.normalX;
				this->rendererNormals[outNormalsIndex + 1] = sourceVertex.normalY;
				this->rendererNormals[outNormalsIndex + 2] = sourceVertex.normalZ;

				this->rendererTexCoords[outTexCoordsIndex] = sourceVertex.texCoordU;

				if (isSideFace)
				{
					this->rendererTexCoords[outTexCoordsIndex + 1] = (sourceVertex.texCoordV == vMax) ? shapeInitCache.raised.vBottom : shapeInitCache.raised.vTop;
				}
				else
				{
					this->rendererTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;
				}

				destinationVertexIndex++;
			}
		}

		break;
	}
	case ArenaVoxelType::Diagonal:
	{
		DebugAssert(shapeInitCache.voxelType == ArenaVoxelType::Diagonal);

		const int diagonalMeshEntryIndex = shapeInitCache.diagonal.isRightDiagonal ? 1 : 0;
		const MeshLibraryEntry &diagonalMeshEntry = meshEntries[diagonalMeshEntryIndex];

		int destinationVertexIndex = 0;
		for (const ObjVertex &sourceVertex : diagonalMeshEntry.vertices)
		{
			const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
			this->rendererPositions[outPositionsIndex] = sourceVertex.positionX;
			this->rendererPositions[outPositionsIndex + 1] = sourceVertex.positionY; // @todo move getScaledVertexY from RenderVoxelChunkManager here
			this->rendererPositions[outPositionsIndex + 2] = sourceVertex.positionZ;
			this->rendererNormals[outNormalsIndex] = sourceVertex.normalX;
			this->rendererNormals[outNormalsIndex + 1] = sourceVertex.normalY;
			this->rendererNormals[outNormalsIndex + 2] = sourceVertex.normalZ;
			this->rendererTexCoords[outTexCoordsIndex] = sourceVertex.texCoordU;
			this->rendererTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;
			destinationVertexIndex++;
		}

		break;
	}
	case ArenaVoxelType::Edge:
	{
		DebugAssert(shapeInitCache.voxelType == ArenaVoxelType::Edge);

		int edgeMeshEntryIndex = -1;
		for (int i = 0; i < meshEntries.getCount(); i++)
		{
			const MeshLibraryEntry &currentEdgeMeshEntry = meshEntries[i];
			const std::optional<VoxelFacing3D> &currentEdgeFacing = currentEdgeMeshEntry.facing;
			const VoxelFacing3D targetEdgeFacing = VoxelUtils::convertFaceTo3D(shapeInitCache.edge.facing);
			if (currentEdgeFacing == targetEdgeFacing)
			{
				edgeMeshEntryIndex = i;
				break;
			}
		}

		const MeshLibraryEntry &edgeMeshEntry = meshEntries[edgeMeshEntryIndex];
		double yMax = std::numeric_limits<double>::lowest();
		for (const ObjVertex &sourceVertex : edgeMeshEntry.vertices)
		{
			yMax = std::max(yMax, sourceVertex.positionY);
		}

		int destinationVertexIndex = 0;
		for (const ObjVertex &sourceVertex : edgeMeshEntry.vertices)
		{
			const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;

			this->rendererPositions[outPositionsIndex] = sourceVertex.positionX;
			this->rendererPositions[outPositionsIndex + 1] = sourceVertex.positionY + shapeInitCache.boxYOffset; // @todo move getScaledVertexY from RenderVoxelChunkManager here
			this->rendererPositions[outPositionsIndex + 2] = sourceVertex.positionZ;

			this->rendererNormals[outNormalsIndex] = sourceVertex.normalX;
			this->rendererNormals[outNormalsIndex + 1] = sourceVertex.normalY;
			this->rendererNormals[outNormalsIndex + 2] = sourceVertex.normalZ;

			this->rendererTexCoords[outTexCoordsIndex] = shapeInitCache.edge.flippedTexCoords ? std::clamp(1.0 - sourceVertex.texCoordU, 0.0, 1.0) : sourceVertex.texCoordU;
			this->rendererTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;

			destinationVertexIndex++;
		}

		break;
	}
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		break;
	}
}

bool VoxelMeshDefinition::isEmpty() const
{
	return this->rendererPositions.empty();
}

int VoxelMeshDefinition::findIndexBufferIndexWithFacing(VoxelFacing3D facing) const
{
	DebugAssert(this->indicesListCount >= this->facingCount);

	for (int i = 0; i < this->facingCount; i++)
	{
		DebugAssertIndex(this->facings, i);
		const VoxelFacing3D currentFacing = this->facings[i];
		if (currentFacing == facing)
		{
			return i;
		}
	}

	return -1;
}

int VoxelMeshDefinition::findTextureSlotIndexWithFacing(VoxelFacing3D facing) const
{
	DebugAssert(this->textureSlotIndexCount >= this->facingCount);

	for (int i = 0; i < this->facingCount; i++)
	{
		DebugAssertIndex(this->facings, i);
		const VoxelFacing3D currentFacing = this->facings[i];
		if (currentFacing == facing)
		{
			return this->textureSlotIndices[i];
		}
	}

	return -1;
}

bool VoxelMeshDefinition::hasFullCoverageOfFacing(VoxelFacing3D facing) const
{
	// @todo eventually this should analyze the mesh + indices, using vertex position checks w/ epsilons

	const int indexBufferIndex = this->findIndexBufferIndexWithFacing(facing);
	return indexBufferIndex >= 0;
}

VoxelShapeDefinition::VoxelShapeDefinition()
{
	// Air by default. Needs the shape defined in case of trigger voxels, but doesn't need a render mesh.
	ArenaShapeInitCache airShapeInitCache;
	airShapeInitCache.initDefaultBoxValues(ArenaVoxelType::None);
	this->initBoxFromClassic(airShapeInitCache, VoxelShapeScaleType::ScaledFromMin, 1.0);
}

void VoxelShapeDefinition::initBoxFromClassic(const ArenaShapeInitCache &shapeInitCache, VoxelShapeScaleType scaleType, double ceilingScale)
{
	this->type = VoxelShapeType::Box;
	this->box.init(shapeInitCache.boxWidth, shapeInitCache.boxHeight, shapeInitCache.boxDepth, shapeInitCache.boxYOffset, shapeInitCache.boxYRotation);
	this->mesh.initClassic(shapeInitCache, scaleType, ceilingScale);
	this->scaleType = scaleType;

	const ArenaVoxelType voxelType = shapeInitCache.voxelType;
	this->allowsBackFaces = ArenaMeshUtils::AllowsBackFacingGeometry(voxelType);
	this->allowsAdjacentDoorFaces = ArenaMeshUtils::AllowsAdjacentDoorFaces(voxelType);
	this->allowsInternalFaceRemoval = ArenaMeshUtils::AllowsInternalFaceRemoval(voxelType);
	this->allowsAdjacentFaceCombining = ArenaMeshUtils::AllowsAdjacentFaceCombining(voxelType);
	this->enablesNeighborGeometry = ArenaMeshUtils::EnablesNeighborVoxelGeometry(voxelType);
	this->isContextSensitive = ArenaMeshUtils::HasContextSensitiveGeometry(voxelType);
	this->isElevatedPlatform = ArenaMeshUtils::IsElevatedPlatform(voxelType);
}
