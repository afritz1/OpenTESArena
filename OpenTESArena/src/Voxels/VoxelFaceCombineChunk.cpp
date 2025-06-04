#include <algorithm>

#include "VoxelChunk.h"
#include "VoxelFaceCombineChunk.h"
#include "VoxelFacing3D.h"

namespace
{
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
		for (const std::pair<ArenaVoxelType, BufferView<const VoxelFacing3D>> &pair : VoxelUtils::VoxelTypeValidFacings)
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
		VoxelInt3 rangeBegin = min;
		if (direction.x != 0)
		{
			rangeBegin.x = max.x;
		}
		else if (direction.y != 0)
		{
			rangeBegin.y = max.y;
		}
		else if (direction.z != 0)
		{
			rangeBegin.z = max.z;
		}

		bool isCombinable = true;
		for (WEInt z = rangeBegin.z; z <= max.z; z++)
		{
			for (int y = rangeBegin.y; y <= max.y; y++)
			{
				for (SNInt x = rangeBegin.x; x <= max.x; x++)
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

	constexpr VoxelInt3 FaceCombineDirections[] =
	{
		VoxelInt3(1, 0, 0),
		VoxelInt3(0, 1, 0),
		VoxelInt3(0, 0, 1)
	};

	// Allows combining adjacent voxels in the plane of the voxel facing only.
	bool IsCombineDirectionValidForFacing(int directionIndex, VoxelFacing3D facing)
	{
		if ((facing == VoxelFacing3D::PositiveX) || (facing == VoxelFacing3D::NegativeX))
		{
			return directionIndex != 0;
		}
		else if ((facing == VoxelFacing3D::PositiveY) || (facing == VoxelFacing3D::NegativeY))
		{
			return directionIndex != 1;
		}
		else if ((facing == VoxelFacing3D::PositiveZ) || (facing == VoxelFacing3D::NegativeZ))
		{
			return directionIndex != 2;
		}
		else
		{
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(facing)));
		}
	}
}

VoxelFacesEntry::VoxelFacesEntry()
{
	this->clear();
}

void VoxelFacesEntry::clear()
{
	std::fill(std::begin(this->combinedFacesIDs), std::end(this->combinedFacesIDs), -1);
}

VoxelFaceCombineDirtyEntry::VoxelFaceCombineDirtyEntry()
{
	std::fill(std::begin(this->dirtyFaces), std::end(this->dirtyFaces), false);
}

VoxelFaceCombineResult::VoxelFaceCombineResult()
{
	this->clear();
}

void VoxelFaceCombineResult::clear()
{
	this->min = VoxelInt3::Zero;
	this->max = VoxelInt3::Zero;
	this->facing = static_cast<VoxelFacing3D>(-1);
}

void VoxelFaceCombineChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->entries.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->entries.fill(VoxelFacesEntry());
}

void VoxelFaceCombineChunk::update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk)
{
	this->dirtyEntries.clear();
	this->dirtyEntries.reserve(dirtyVoxels.getCount());

	// Free any combined faces associated with the dirty voxels.
	for (const VoxelInt3 voxel : dirtyVoxels)
	{
		VoxelFacesEntry &facesEntry = this->entries.get(voxel.x, voxel.y, voxel.z);

		auto dirtyEntryIter = this->dirtyEntries.find(voxel);
		if (dirtyEntryIter == this->dirtyEntries.end())
		{
			dirtyEntryIter = this->dirtyEntries.emplace(voxel, VoxelFaceCombineDirtyEntry()).first;
		}

		VoxelFaceCombineDirtyEntry &dirtyEntry = dirtyEntryIter->second;
		std::fill(std::begin(dirtyEntry.dirtyFaces), std::end(dirtyEntry.dirtyFaces), true);

		for (int faceIndex = 0; faceIndex < VoxelUtils::FACE_COUNT; faceIndex++)
		{
			const VoxelFacing3D facing = VoxelUtils::getFaceIndexFacing(faceIndex);

			VoxelFaceCombineResultID faceCombineResultID = facesEntry.combinedFacesIDs[faceIndex];
			if (faceCombineResultID >= 0)
			{
				VoxelFaceCombineResult &faceCombineResult = this->combinedFacesPool.get(faceCombineResultID);

				for (WEInt currentZ = faceCombineResult.min.z; currentZ <= faceCombineResult.max.z; currentZ++)
				{
					for (int currentY = faceCombineResult.min.y; currentY <= faceCombineResult.max.y; currentY++)
					{
						for (SNInt currentX = faceCombineResult.min.x; currentX <= faceCombineResult.max.x; currentX++)
						{
							VoxelFacesEntry &currentFacesEntry = this->entries.get(currentX, currentY, currentZ);
							VoxelFaceCombineResultID &currentFaceCombineResultID = currentFacesEntry.combinedFacesIDs[faceIndex];
							if (currentFaceCombineResultID == faceCombineResultID)
							{
								currentFaceCombineResultID = -1;

								const VoxelInt3 currentVoxel(currentX, currentY, currentZ);
								auto currentDirtyEntryIter = this->dirtyEntries.find(currentVoxel);
								if (currentDirtyEntryIter == this->dirtyEntries.end())
								{
									currentDirtyEntryIter = this->dirtyEntries.emplace(currentVoxel, VoxelFaceCombineDirtyEntry()).first;
								}

								VoxelFaceCombineDirtyEntry &currentDirtyEntry = currentDirtyEntryIter->second;
								currentDirtyEntry.dirtyFaces[faceIndex] = true;
							}
						}
					}
				}

				this->combinedFacesPool.free(faceCombineResultID);
			}
		}
	}

	// Combine dirty faces together where possible.
	for (auto iter = this->dirtyEntries.begin(); iter != this->dirtyEntries.end(); iter++)
	{
		const VoxelInt3 voxel = iter->first;
		const VoxelShapeDefID shapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);
		if (shapeDef.mesh.isEmpty())
		{
			continue;
		}

		VoxelFaceCombineDirtyEntry &dirtyEntry = iter->second;
		VoxelFacesEntry &facesEntry = this->entries.get(voxel.x, voxel.y, voxel.z);

		for (int faceIndex = 0; faceIndex < VoxelUtils::FACE_COUNT; faceIndex++)
		{
			const VoxelFacing3D facing = VoxelUtils::getFaceIndexFacing(faceIndex);
			const bool isFaceDirty = dirtyEntry.dirtyFaces[faceIndex];
			if (!isFaceDirty)
			{
				continue;
			}

			VoxelFaceCombineResultID faceCombineResultID;
			if (!this->combinedFacesPool.tryAlloc(&faceCombineResultID))
			{
				DebugLogErrorFormat("Couldn't allocate voxel face combine result ID (voxel %s).", voxel.toString().c_str());
				continue;
			}

			VoxelFaceCombineResult &faceCombineResult = this->combinedFacesPool.get(faceCombineResultID);
			faceCombineResult.min = voxel;
			faceCombineResult.max = voxel;
			faceCombineResult.facing = facing;

			DebugAssert(facesEntry.combinedFacesIDs[faceIndex] == -1);
			facesEntry.combinedFacesIDs[faceIndex] = faceCombineResultID;

			for (int combineDirectionIndex = 0; combineDirectionIndex < static_cast<int>(std::size(FaceCombineDirections)); combineDirectionIndex++)
			{
				// Only combine in this facing's plane.
				if (!IsCombineDirectionValidForFacing(combineDirectionIndex, facing))
				{
					continue;
				}

				const VoxelInt3 combineDirection = FaceCombineDirections[combineDirectionIndex];
				while (true)
				{
					const VoxelInt3 adjacentVoxel = faceCombineResult.max + combineDirection;
					const bool canTestAdjacentVoxel = voxelChunk.isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
					if (!canTestAdjacentVoxel)
					{
						// Ran into chunk edge.
						break;
					}

					if (!IsAdjacentFaceRangeCombinable(voxel, faceCombineResult.max, combineDirection, facing, voxelChunk))
					{
						// One or more voxels in the adjacent range aren't similar enough to the start voxel.
						break;
					}

					VoxelFacesEntry &adjacentFacesEntry = this->entries.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
					if (adjacentFacesEntry.combinedFacesIDs[faceIndex] >= 0)
					{
						// Already combined with something else.
						break;
					}

					// Add this face to the combined face and un-dirty it.
					adjacentFacesEntry.combinedFacesIDs[faceIndex] = faceCombineResultID;
					const auto adjacentDirtyEntryIter = this->dirtyEntries.find(adjacentVoxel);
					if (adjacentDirtyEntryIter != this->dirtyEntries.end())
					{
						VoxelFaceCombineDirtyEntry &adjacentDirtyEntry = adjacentDirtyEntryIter->second;
						adjacentDirtyEntry.dirtyFaces[faceIndex] = false;
					}

					faceCombineResult.max = faceCombineResult.max + combineDirection;
				}
			}
		}
	}
}

void VoxelFaceCombineChunk::clear()
{
	Chunk::clear();
	this->dirtyEntries.clear();
	this->combinedFacesPool.clear();
	this->entries.clear();
}
