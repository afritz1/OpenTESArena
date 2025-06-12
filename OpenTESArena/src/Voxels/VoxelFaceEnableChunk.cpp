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

	bool IsVoxelFaceOpaque(const VoxelShadingDefinition &shadingDef, VoxelFacing3D facing)
	{
		// @todo: currently this is very pessimistic. Ideally need a way to map mesh faces (or triangles) to pixel shader indices

		bool isOpaque = true;
		for (int i = 0; i < shadingDef.pixelShaderCount; i++)
		{
			const PixelShaderType pixelShaderType = shadingDef.pixelShaderTypes[i];
			if (!RenderShaderUtils::isOpaque(pixelShaderType))
			{
				isOpaque = false;
				break;
			}
		}

		return isOpaque;
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

void VoxelFaceEnableChunk::update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk)
{
	for (const VoxelInt3 voxel : dirtyVoxels)
	{
		VoxelFaceEnableEntry &faceEnableEntry = this->entries.get(voxel.x, voxel.y, voxel.z);

		const VoxelShapeDefID shapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);		
		if (!shapeDef.allowsInternalFaceRemoval)
		{
			// This shape doesn't participate in face enabling/disabling.
			faceEnableEntry.fill(true);
			continue;
		}

		const VoxelMeshDefinition &meshDef = shapeDef.mesh;
		const VoxelShadingDefID shadingDefID = voxelChunk.getShadingDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefinition &shadingDef = voxelChunk.getShadingDef(shadingDefID);

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

			if (!IsVoxelFaceOpaque(shadingDef, facing))
			{
				// Non-opaque faces not important enough.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			// Disable this voxel's face if the adjacent one blocks it completely.
			const VoxelShapeDefID adjacentShapeDefID = voxelChunk.getShapeDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
			const VoxelShapeDefinition &adjacentShapeDef = voxelChunk.getShapeDef(adjacentShapeDefID);
			const VoxelMeshDefinition &adjacentMeshDef = adjacentShapeDef.mesh;
			const VoxelFacing3D adjacentFacing = VoxelUtils::getOppositeFacing(facing);
			if (!adjacentMeshDef.hasFullCoverageOfFacing(adjacentFacing))
			{
				// Adjacent face doesn't get full coverage from its mesh, not important enough.
				faceEnableEntry.enabledFaces[faceIndex] = true;
				continue;
			}

			const VoxelShadingDefID adjacentShadingDefID = voxelChunk.getShadingDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
			const VoxelShadingDefinition &adjacentShadingDef = voxelChunk.getShadingDef(adjacentShadingDefID);
			const bool isAdjacentFaceBlocking = IsVoxelFaceOpaque(adjacentShadingDef, adjacentFacing);
			faceEnableEntry.enabledFaces[faceIndex] = !isAdjacentFaceBlocking;
		}
	}
}

void VoxelFaceEnableChunk::clear()
{
	Chunk::clear();
	this->entries.clear();
}
