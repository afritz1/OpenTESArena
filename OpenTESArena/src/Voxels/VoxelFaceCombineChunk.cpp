#include <algorithm>

#include "VoxelChunk.h"
#include "VoxelFaceCombineChunk.h"
#include "VoxelFacing3D.h"

namespace
{
	constexpr VoxelFacing3D ValidFacings_Wall[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveY, VoxelFacing3D::NegativeY, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };
	constexpr VoxelFacing3D ValidFacings_Floor[] = { VoxelFacing3D::PositiveY };
	constexpr VoxelFacing3D ValidFacings_Ceiling[] = { VoxelFacing3D::NegativeY };
	constexpr VoxelFacing3D ValidFacings_Raised[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveY, VoxelFacing3D::NegativeY, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };
	constexpr VoxelFacing3D ValidFacings_TransparentWall[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };

	const std::pair<ArenaVoxelType, BufferView<const VoxelFacing3D>> VoxelTypeValidFacings[] =
	{
		{ ArenaVoxelType::None, BufferView<const VoxelFacing3D>() },
		{ ArenaVoxelType::Wall, ValidFacings_Wall },
		{ ArenaVoxelType::Floor, ValidFacings_Floor },
		{ ArenaVoxelType::Ceiling, ValidFacings_Ceiling },
		{ ArenaVoxelType::Raised, ValidFacings_Raised },
		{ ArenaVoxelType::Diagonal, BufferView<const VoxelFacing3D>() }, // Needs more than facing check
		{ ArenaVoxelType::TransparentWall, ValidFacings_TransparentWall },
		{ ArenaVoxelType::Edge, BufferView<const VoxelFacing3D>() }, // Depends on edge definition
		{ ArenaVoxelType::Chasm, BufferView<const VoxelFacing3D>() }, // Depends on chasm wall instance
		{ ArenaVoxelType::Door, BufferView<const VoxelFacing3D>() } // Not worth combining
	};

	VoxelFacing3D GetFaceIndexFacing(int faceIndex)
	{
		DebugAssert(faceIndex >= 0);
		DebugAssert(faceIndex < VoxelFacesEntry::FACE_COUNT);
		return static_cast<VoxelFacing3D>(faceIndex);
	}

	bool IsAdjacentFaceCombinable(const VoxelInt3 &voxel, const VoxelInt3 &direction, VoxelFacing3D facing, const VoxelChunk &voxelChunk)
	{
		const VoxelInt3 adjacentVoxel = voxel + direction;
		if (!voxelChunk.isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z))
		{
			return false;
		}

		const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefID adjacentVoxelShapeDefID = voxelChunk.getShapeDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		if (voxelShapeDefID != adjacentVoxelShapeDefID)
		{
			return false;
		}

		const VoxelTextureDefID voxelTextureDefID = voxelChunk.getTextureDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTextureDefID adjacentVoxelTextureDefID = voxelChunk.getTextureDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		if (voxelTextureDefID != adjacentVoxelTextureDefID)
		{
			return false;
		}

		const VoxelShadingDefID voxelShadingDefID = voxelChunk.getShadingDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefID adjacentVoxelShadingDefID = voxelChunk.getShadingDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		if (voxelShadingDefID != adjacentVoxelShadingDefID)
		{
			return false;
		}

		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefID adjacentVoxelTraitsDefID = voxelChunk.getTraitsDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		if (voxelTraitsDefID != adjacentVoxelTraitsDefID)
		{
			return false;
		}

		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
		const ArenaVoxelType voxelTraitsDefType = voxelTraitsDef.type;
		BufferView<const VoxelFacing3D> validFacings;
		for (const std::pair<ArenaVoxelType, BufferView<const VoxelFacing3D>> &pair : VoxelTypeValidFacings)
		{
			if (pair.first == voxelTraitsDefType)
			{
				validFacings = pair.second;
				break;
			}
		}

		if (validFacings.isValid())
		{
			bool isValidFacing = false;
			for (const VoxelFacing3D validFacing : validFacings)
			{
				if (validFacing == facing)
				{
					isValidFacing = true;
					break;
				}
			}

			if (!isValidFacing)
			{
				return false;
			}
		}
		else
		{
			// Filter out special case voxel types.
			if (voxelTraitsDefType == ArenaVoxelType::None)
			{
				return false;
			}
			else if (voxelTraitsDefType == ArenaVoxelType::Diagonal)
			{
				// @todo diagonal adjacency support
				return false;
			}
			else if (voxelTraitsDefType == ArenaVoxelType::Edge)
			{
				// Can assume same edge def due to equal traits IDs.
			}
			else if (voxelTraitsDefType == ArenaVoxelType::Chasm)
			{
				int chasmWallInstIndex, adjacentChasmWallInstIndex;
				bool hasChasmWallInst = voxelChunk.tryGetChasmWallInstIndex(voxel.x, voxel.y, voxel.z, &chasmWallInstIndex);
				bool hasAdjacentChasmWallInst = voxelChunk.tryGetChasmWallInstIndex(voxel.x, voxel.y, voxel.z, &adjacentChasmWallInstIndex);
				if (hasChasmWallInst != hasAdjacentChasmWallInst)
				{
					return false;
				}

				if (hasChasmWallInst && hasAdjacentChasmWallInst)
				{
					BufferView<const VoxelChasmWallInstance> chasmWallInsts = voxelChunk.getChasmWallInsts();
					const VoxelChasmWallInstance &chasmWallInst = chasmWallInsts.get(chasmWallInstIndex);
					const VoxelChasmWallInstance &adjacentChasmWallInst = chasmWallInsts.get(adjacentChasmWallInstIndex);

					if (((facing == VoxelFacing3D::PositiveX) && !(chasmWallInst.north && adjacentChasmWallInst.north)) ||
						((facing == VoxelFacing3D::NegativeX) && !(chasmWallInst.south && adjacentChasmWallInst.south)) ||
						((facing == VoxelFacing3D::PositiveZ) && !(chasmWallInst.west && adjacentChasmWallInst.west)) ||
						((facing == VoxelFacing3D::NegativeZ) && !(chasmWallInst.east && adjacentChasmWallInst.east)))
					{
						return false;
					}
				}
			}
			else if (voxelTraitsDefType == ArenaVoxelType::Door)
			{
				// Doors not allowed
				return false;
			}
			else
			{
				DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelTraitsDefType)));
			}
		}

		int fadeAnimInstIndex;
		if (voxelChunk.tryGetFadeAnimInstIndex(voxel.x, voxel.y, voxel.z, &fadeAnimInstIndex) ||
			voxelChunk.tryGetFadeAnimInstIndex(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z, &fadeAnimInstIndex))
		{
			return false;
		}

		return true;
	}

	bool IsAdjacentFaceRangeCombinable(const VoxelInt3 &min, const VoxelInt3 &max, const VoxelInt3 &direction, VoxelFacing3D facing, const VoxelChunk &voxelChunk)
	{
		bool isCombinable = true;
		for (WEInt z = min.z; z <= max.z; z++)
		{
			for (int y = min.y; y <= max.y; y++)
			{
				for (SNInt x = min.x; x <= max.x; x++)
				{
					const VoxelInt3 voxel(x, y, z);
					if (!IsAdjacentFaceCombinable(voxel, direction, facing, voxelChunk))
					{
						isCombinable = false;
						break;
					}
				}
			}
		}

		return isCombinable;
	}
}

VoxelFacesEntry::VoxelFacesEntry()
{
	this->clear();
}

void VoxelFacesEntry::clear()
{
	std::fill(std::begin(this->combinedFacesIndices), std::end(this->combinedFacesIndices), -1);
}

VoxelFaceCombineResult::VoxelFaceCombineResult()
{
	this->clear();
}

void VoxelFaceCombineResult::clear()
{
	this->facing = static_cast<VoxelFacing3D>(-1);
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->lightingType = static_cast<RenderLightingType>(-1);
}

void VoxelFaceCombineChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->entries.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->entries.fill(VoxelFacesEntry());
}

void VoxelFaceCombineChunk::update(const BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk)
{
	// @todo i think the reason the voxel is dirty matters. if it's a fade anim starting then it should NOT combine that one, it should BREAK it and rerun combining on ADJACENT ones
	// - or like it just needs to check if its current Is Adjacent Stuff Combinable is still valid.

	// @todo search through all faces of all dirty voxels, try to make combined face entries

	for (const VoxelInt3 voxel : dirtyVoxels)
	{
		VoxelFacesEntry &facesEntry = this->entries.get(voxel.x, voxel.y, voxel.z);

		for (int faceIndex = 0; faceIndex < VoxelFacesEntry::FACE_COUNT; faceIndex++)
		{
			const VoxelFacing3D facing = GetFaceIndexFacing(faceIndex);

			// @todo if this voxel is dirty, need to check if we already combined it with adjacent ones by comparing combinedFaceIndex between them

			int &combinedFaceIndex = facesEntry.combinedFacesIndices[faceIndex];
			if (combinedFaceIndex >= 0)
			{
				// Is part of an existing face, may need to break it up.

			}
			else
			{
				// Not combined into any faces yet.
			}
		}
	}
}

void VoxelFaceCombineChunk::clear()
{
	Chunk::clear();
	this->combinedFaces.clear();
	this->entries.clear();
}
