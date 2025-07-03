#include <algorithm>

#include "VoxelFaceEnableChunk.h"
#include "VoxelChunk.h"
#include "VoxelFacing.h"
#include "../Rendering/RenderShaderUtils.h"

namespace
{
	constexpr Int3 FaceEnableDirections[] =
	{
		Int3(1, 0, 0),
		Int3(-1, 0, 0),
		Int3(0, 1, 0),
		Int3(0, -1, 0),
		Int3(0, 0, 1),
		Int3(0, 0, -1)
	};

	bool IsVoxelFaceOpaque(VoxelFacing3D facing, const VoxelMeshDefinition &meshDef, const VoxelShadingDefinition &shadingDef)
	{
		const int textureSlotIndex = meshDef.findTextureSlotIndexWithFacing(facing);
		if (textureSlotIndex < 0)
		{
			return false;
		}

		DebugAssert(textureSlotIndex < shadingDef.pixelShaderCount);
		DebugAssertIndex(shadingDef.pixelShaderTypes, textureSlotIndex);
		const PixelShaderType pixelShaderType = shadingDef.pixelShaderTypes[textureSlotIndex];
		return RenderShaderUtils::isOpaque(pixelShaderType);
	}
}

VoxelFaceEnableEntry::VoxelFaceEnableEntry()
{
	this->fill(false);
}

void VoxelFaceEnableEntry::fill(bool enabled)
{
	std::fill(std::begin(this->enabledFaces), std::end(this->enabledFaces), enabled);
}

void VoxelFaceEnableChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->entries.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->entries.fill(VoxelFaceEnableEntry());
}

void VoxelFaceEnableChunk::update(Span<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk)
{
	for (const VoxelInt3 voxel : dirtyVoxels)
	{
		VoxelFaceEnableEntry &faceEnableEntry = this->entries.get(voxel.x, voxel.y, voxel.z);

		const VoxelShapeDefID shapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.shapeDefs[shapeDefID];
		if (!shapeDef.allowsInternalFaceRemoval)
		{
			// This shape doesn't participate in face enabling/disabling.
			faceEnableEntry.fill(true);
			continue;
		}

		VoxelChasmDefID chasmDefID;
		if (voxelChunk.tryGetChasmDefID(voxel.x, voxel.y, voxel.z, &chasmDefID))
		{
			// Chasm face enabling is determined by chasm wall instance.
			const int positiveXFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::PositiveX);
			const int negativeXFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::NegativeX);
			const int positiveYFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::PositiveY);
			const int negativeYFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::NegativeY);
			const int positiveZFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::PositiveZ);
			const int negativeZFaceIndex = VoxelUtils::getFacingIndex(VoxelFacing3D::NegativeZ);
			faceEnableEntry.enabledFaces[positiveYFaceIndex] = false;
			faceEnableEntry.enabledFaces[negativeYFaceIndex] = true;

			int chasmWallInstIndex;
			if (voxelChunk.tryGetChasmWallInstIndex(voxel.x, voxel.y, voxel.z, &chasmWallInstIndex))
			{
				const VoxelChasmWallInstance &chasmWallInst = voxelChunk.chasmWallInsts[chasmWallInstIndex];				
				faceEnableEntry.enabledFaces[positiveXFaceIndex] = chasmWallInst.south;
				faceEnableEntry.enabledFaces[negativeXFaceIndex] = chasmWallInst.north;
				faceEnableEntry.enabledFaces[positiveZFaceIndex] = chasmWallInst.west;
				faceEnableEntry.enabledFaces[negativeZFaceIndex] = chasmWallInst.east;
			}
			else
			{
				faceEnableEntry.enabledFaces[positiveXFaceIndex] = false;
				faceEnableEntry.enabledFaces[negativeXFaceIndex] = false;
				faceEnableEntry.enabledFaces[positiveZFaceIndex] = false;
				faceEnableEntry.enabledFaces[negativeZFaceIndex] = false;
			}

			continue;
		}

		const VoxelMeshDefinition &meshDef = shapeDef.mesh;
		const VoxelShadingDefID shadingDefID = voxelChunk.shadingDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefinition &shadingDef = voxelChunk.shadingDefs[shadingDefID];

		for (int faceIndex = 0; faceIndex < VoxelUtils::FACE_COUNT; faceIndex++)
		{
			DebugAssertIndex(FaceEnableDirections, faceIndex);
			const Int3 direction = FaceEnableDirections[faceIndex];
			const VoxelInt3 adjacentVoxel = voxel + direction;
			if (!voxelChunk.isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z))
			{
				// Chunk edge faces are always enabled for simplicity.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			const VoxelFacing3D facing = VoxelUtils::getFaceIndexFacing(faceIndex);
			if (!meshDef.hasFullCoverageOfFacing(facing))
			{
				// This face doesn't get full coverage from its own mesh, not important enough.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			if (!IsVoxelFaceOpaque(facing, meshDef, shadingDef))
			{
				// Non-opaque faces not important enough.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			const VoxelShapeDefID adjacentShapeDefID = voxelChunk.shapeDefIDs.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
			const VoxelShapeDefinition &adjacentShapeDef = voxelChunk.shapeDefs[adjacentShapeDefID];
			const bool canAdjacentShapeDisableNeighborFaces = adjacentShapeDef.allowsInternalFaceRemoval;
			if (!canAdjacentShapeDisableNeighborFaces)
			{
				// Adjacent face doesn't participate in face enabling/disabling.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			const VoxelMeshDefinition &adjacentMeshDef = adjacentShapeDef.mesh;
			const VoxelFacing3D adjacentFacing = VoxelUtils::getOppositeFacing(facing);
			if (!adjacentMeshDef.hasFullCoverageOfFacing(adjacentFacing))
			{
				// Adjacent face doesn't get full coverage from its mesh, not important enough.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			const VoxelShadingDefID adjacentShadingDefID = voxelChunk.shadingDefIDs.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
			const VoxelShadingDefinition &adjacentShadingDef = voxelChunk.shadingDefs[adjacentShadingDefID];
			const bool isAdjacentFaceBlocking = IsVoxelFaceOpaque(adjacentFacing, adjacentMeshDef, adjacentShadingDef);
			faceEnableEntry.enabledFaces[faceIndex] = !isAdjacentFaceBlocking;
		}
	}
}

void VoxelFaceEnableChunk::clear()
{
	Chunk::clear();
	this->entries.clear();
}
