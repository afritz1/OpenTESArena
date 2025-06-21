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

	this->indicesListCount = meshEntries.getCount();
	this->facingCount = 0;
	this->textureSlotIndexCount = meshEntries.getCount();

	int vertexCount = 0;
	for (int i = 0; i < meshEntries.getCount(); i++)
	{
		const MeshLibraryEntry &entry = meshEntries[i];
		vertexCount += static_cast<int>(entry.vertices.size());

		Span<const int32_t> meshIndices = entry.vertexIndices;
		std::vector<int32_t> &indicesList = this->indicesLists[i];
		indicesList.resize(meshIndices.getCount());
		std::copy(meshIndices.begin(), meshIndices.end(), indicesList.begin());

		if (entry.facing.has_value())
		{
			this->facings[this->facingCount] = *entry.facing;
			this->facingCount++;
		}

		this->textureSlotIndices[i] = entry.textureSlotIndex;
	}

	const int rendererVertexPositionComponentCount = vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	const int rendererVertexNormalComponentCount = vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	const int rendererVertexTexCoordComponentCount = vertexCount * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	this->rendererPositions.resize(rendererVertexPositionComponentCount);
	this->rendererNormals.resize(rendererVertexNormalComponentCount);
	this->rendererTexCoords.resize(rendererVertexTexCoordComponentCount);

	// @todo: use ceilingScale for the position buffers here FOR NON-COMBINED MESHES
	// - do the MeshUtils::getScaledVertexY(srcY, scaleType, ceilingScale); to the rendererPositions
	// - ^^ MeshUtils::createVoxelFaceQuadPositionsModelSpace() is doing this for combined meshes in RenderVoxelChunkManager.cpp
	//DebugNotImplemented();

	// @todo only need the special WriteIndexBuffers() functions if order is needed for voxel texture def stuff (which idk, need to use the textureSlotIndex at some point)

	switch (voxelType)
	{
		case ArenaVoxelType::None:
			// Nothing
			break;
		case ArenaVoxelType::Wall:
			ArenaMeshUtils::writeWallRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			//WriteIndexBuffersSidesBottomTop(ArenaVoxelType::Wall, this->indicesLists[0], this->indicesLists[1], this->indicesLists[2]);
			break;
		case ArenaVoxelType::Floor:
			ArenaMeshUtils::writeFloorRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Ceiling:
			ArenaMeshUtils::writeCeilingRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Raised:
			ArenaMeshUtils::writeRaisedRendererGeometryBuffers(shapeInitCache.boxYOffset, shapeInitCache.boxHeight, shapeInitCache.raised.vBottom, shapeInitCache.raised.vTop, this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Diagonal:
			ArenaMeshUtils::writeDiagonalRendererGeometryBuffers(shapeInitCache.diagonal.isRightDiagonal, this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::TransparentWall:
			ArenaMeshUtils::writeTransparentWallRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Edge:
			ArenaMeshUtils::writeEdgeRendererGeometryBuffers(shapeInitCache.boxYOffset, shapeInitCache.edge.facing, shapeInitCache.edge.flippedTexCoords, this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Chasm:
			// @todo we only want the chasm floor index buffer written above, right? don't want all chasm walls? idk
			ArenaMeshUtils::writeChasmRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
			break;
		case ArenaVoxelType::Door:
			ArenaMeshUtils::writeDoorRendererGeometryBuffers(this->rendererPositions, this->rendererNormals, this->rendererTexCoords);
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
