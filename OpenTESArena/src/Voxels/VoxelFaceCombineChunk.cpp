#include <algorithm>

#include "VoxelChunk.h"
#include "VoxelFaceCombineChunk.h"
#include "VoxelFaceEnableChunk.h"
#include "VoxelFacing.h"

#include "components/utilities/BufferView3D.h"

namespace
{
	bool IsAdjacentFaceCombinable(const VoxelInt3 &voxel, const VoxelInt3 &direction, VoxelFacing3D facing, BufferView3D<const VoxelFacesEntry> facesEntry,
		const VoxelChunk &voxelChunk, const VoxelFaceEnableChunk &faceEnableChunk)
	{
		const VoxelInt3 adjacentVoxel = voxel + direction;
		if (!voxelChunk.isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z))
		{
			return false;
		}

		const VoxelFaceEnableEntry &adjacentFaceEnableEntry = faceEnableChunk.entries.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		const int adjacentFaceIndex = VoxelUtils::getFacingIndex(facing);
		const bool isAdjacentFaceEnabled = adjacentFaceEnableEntry.enabledFaces[adjacentFaceIndex];
		if (!isAdjacentFaceEnabled)
		{
			return false;
		}

		const VoxelFacesEntry &adjacentFacesEntry = facesEntry.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		const bool isAdjacentFaceAlreadyCombined = adjacentFacesEntry.combinedFacesIDs[adjacentFaceIndex] >= 0;
		if (isAdjacentFaceAlreadyCombined)
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
				bool hasAdjacentChasmWallInst = voxelChunk.tryGetChasmWallInstIndex(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z, &adjacentChasmWallInstIndex);
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

	bool IsAdjacentFaceRangeCombinable(const VoxelInt3 &rangeBegin, const VoxelInt3 &rangeEnd, const VoxelInt3 &direction, VoxelFacing3D facing,
		BufferView3D<const VoxelFacesEntry> facesEntry, const VoxelChunk &voxelChunk, const VoxelFaceEnableChunk &faceEnableChunk)
	{
		// The input range should be a 1D span of voxels, we only care about the next 1D span adjacent to it.
		bool isCombinable = true;
		for (WEInt z = rangeBegin.z; z <= rangeEnd.z; z++)
		{
			for (int y = rangeBegin.y; y <= rangeEnd.y; y++)
			{
				for (SNInt x = rangeBegin.x; x <= rangeEnd.x; x++)
				{
					const VoxelInt3 voxel(x, y, z);
					if (!IsAdjacentFaceCombinable(voxel, direction, facing, facesEntry, voxelChunk, faceEnableChunk))
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

void VoxelFaceCombineChunk::update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk, const VoxelFaceEnableChunk &faceEnableChunk)
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

			const VoxelFaceCombineResultID faceCombineResultID = facesEntry.combinedFacesIDs[faceIndex];
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
		VoxelFaceCombineDirtyEntry &dirtyEntry = iter->second;

		const VoxelShapeDefID shapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);
		if (shapeDef.mesh.isEmpty())
		{
			// No need to dirty air.
			std::fill(std::begin(dirtyEntry.dirtyFaces), std::end(dirtyEntry.dirtyFaces), false);
			continue;
		}

		const VoxelTraitsDefID traitsDefID = voxelChunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefinition &traitsDef = voxelChunk.getTraitsDef(traitsDefID);
		const ArenaVoxelType voxelType = traitsDef.type;

		VoxelFacesEntry &facesEntry = this->entries.get(voxel.x, voxel.y, voxel.z);
		const VoxelFaceEnableEntry &faceEnableEntry = faceEnableChunk.entries.get(voxel.x, voxel.y, voxel.z);

		for (int faceIndex = 0; faceIndex < VoxelUtils::FACE_COUNT; faceIndex++)
		{
			const bool isFaceDirty = dirtyEntry.dirtyFaces[faceIndex];
			if (!isFaceDirty)
			{
				continue;
			}

			const bool isFaceEnabled = faceEnableEntry.enabledFaces[faceIndex];
			if (!isFaceEnabled)
			{
				continue;
			}

			const VoxelFacing3D facing = VoxelUtils::getFaceIndexFacing(faceIndex);
			if (!VoxelUtils::isVoxelTypeMeshCoveringFacing(voxelType, facing))
			{
				continue;
			}

			DebugAssert(facesEntry.combinedFacesIDs[faceIndex] == -1);

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
					// Calculate the 1D span of voxels whose adjacent voxels will be checked.
					VoxelInt3 rangeBegin = voxel;
					if (combineDirection.x != 0)
					{
						rangeBegin.x = faceCombineResult.max.x;
					}
					else if (combineDirection.y != 0)
					{
						rangeBegin.y = faceCombineResult.max.y;
					}
					else if (combineDirection.z != 0)
					{
						rangeBegin.z = faceCombineResult.max.z;
					}

					const VoxelInt3 rangeEnd = faceCombineResult.max;

					// Set all faces in this range to be part of the combined face.
					for (WEInt combinedFaceZ = rangeBegin.z; combinedFaceZ <= rangeEnd.z; combinedFaceZ++)
					{
						for (int combinedFaceY = rangeBegin.y; combinedFaceY <= rangeEnd.y; combinedFaceY++)
						{
							for (SNInt combinedFaceX = rangeBegin.x; combinedFaceX <= rangeEnd.x; combinedFaceX++)
							{
								const VoxelInt3 combinedFaceVoxel(combinedFaceX, combinedFaceY, combinedFaceZ);
								VoxelFacesEntry &combinedFacesEntry = this->entries.get(combinedFaceVoxel.x, combinedFaceVoxel.y, combinedFaceVoxel.z);
								combinedFacesEntry.combinedFacesIDs[faceIndex] = faceCombineResultID;

								const auto combinedDirtyEntryIter = this->dirtyEntries.find(combinedFaceVoxel);
								if (combinedDirtyEntryIter != this->dirtyEntries.end())
								{
									VoxelFaceCombineDirtyEntry &combinedDirtyEntry = combinedDirtyEntryIter->second;
									combinedDirtyEntry.dirtyFaces[faceIndex] = false;
								}
							}
						}
					}

					if (!IsAdjacentFaceRangeCombinable(rangeBegin, rangeEnd, combineDirection, facing, this->entries, voxelChunk, faceEnableChunk))
					{
						// One or more voxels in the adjacent range aren't able to combine.
						break;
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
