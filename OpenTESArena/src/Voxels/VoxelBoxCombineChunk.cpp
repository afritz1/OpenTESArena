#include <algorithm>

#include "VoxelBoxCombineChunk.h"
#include "VoxelChunk.h"

#include "components/utilities/Span3D.h"

namespace
{
	bool IsAdjacentBoxCombinable(const VoxelInt3 &voxel, const VoxelInt3 &direction, Span3D<const VoxelBoxCombineResultID> entryIDs, const VoxelChunk &voxelChunk)
	{
		const VoxelInt3 adjacentVoxel = voxel + direction;
		if (!voxelChunk.isValidVoxel(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z))
		{
			return false;
		}

		const VoxelBoxCombineResultID adjacentEntryID = entryIDs.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		const bool isAdjacentBoxAlreadyCombined = adjacentEntryID >= 0;
		if (isAdjacentBoxAlreadyCombined)
		{
			return false;
		}

		const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefID adjacentVoxelShapeDefID = voxelChunk.shapeDefIDs.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.shapeDefs[voxelShapeDefID];
		const VoxelShapeDefinition &adjacentVoxelShapeDef = voxelChunk.shapeDefs[adjacentVoxelShapeDefID];
		if (!voxelShapeDef.allowsAdjacentFaceCombining || !adjacentVoxelShapeDef.allowsAdjacentFaceCombining)
		{
			return false;
		}

		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefID adjacentVoxelTraitsDefID = voxelChunk.traitsDefIDs.get(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];
		const VoxelTraitsDefinition &adjacentVoxelTraitsDef = voxelChunk.traitsDefs[adjacentVoxelTraitsDefID];
		const ArenaVoxelType voxelType = voxelTraitsDef.type;
		const ArenaVoxelType adjacentVoxelType = adjacentVoxelTraitsDef.type;
		if (voxelType != adjacentVoxelType)
		{
			return false;
		}

		DebugAssert(voxelShapeDef.type == VoxelShapeType::Box);
		const VoxelBoxShapeDefinition &boxShapeDef = voxelShapeDef.box;
		const VoxelBoxShapeDefinition &adjacentBoxShapeDef = adjacentVoxelShapeDef.box;
		if (boxShapeDef != adjacentBoxShapeDef)
		{
			return false;
		}

		switch (voxelType)
		{
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Raised:
			break;
		case ArenaVoxelType::Wall:
		{
			VoxelTransitionDefID transitionDefID;
			VoxelTransitionDefID adjacentTransitionDefID;
			const bool isTransitionVoxel = voxelChunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID);
			const bool adjacentIsTransitionVoxel = voxelChunk.tryGetTransitionDefID(adjacentVoxel.x, adjacentVoxel.y, adjacentVoxel.z, &adjacentTransitionDefID);
			if (isTransitionVoxel != adjacentIsTransitionVoxel)
			{
				return false;
			}
			else if (isTransitionVoxel)
			{
				const TransitionDefinition &transitionDef = voxelChunk.transitionDefs[transitionDefID];
				const TransitionDefinition &adjacentTransitionDef = voxelChunk.transitionDefs[adjacentTransitionDefID];
				if (transitionDef.type != adjacentTransitionDef.type)
				{
					return false;
				}
			}

			break;
		}
		case ArenaVoxelType::Edge:
		{
			const VoxelTraitsEdgeDefinition &edgeDef = voxelTraitsDef.edge;
			const VoxelTraitsEdgeDefinition &adjacentEdgeDef = adjacentVoxelTraitsDef.edge;
			if ((edgeDef.facing != adjacentEdgeDef.facing) || (edgeDef.collider != adjacentEdgeDef.collider))
			{
				return false;
			}

			break;
		}
		case ArenaVoxelType::TransparentWall:
		{
			const VoxelTraitsTransparentWallDefinition &transparentWallDef = voxelTraitsDef.transparentWall;
			const VoxelTraitsTransparentWallDefinition &adjacentTransparentWallDef = adjacentVoxelTraitsDef.transparentWall;
			if (transparentWallDef.collider != adjacentTransparentWallDef.collider)
			{
				return false;
			}

			break;
		}
		case ArenaVoxelType::Chasm:
		{
			const VoxelTraitsChasmDefinition &chasmDef = voxelTraitsDef.chasm;
			const VoxelTraitsChasmDefinition &adjacentChasmDef = adjacentVoxelTraitsDef.chasm;
			if (chasmDef.type != adjacentChasmDef.type)
			{
				return false;
			}

			break;
		}
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
			break;
		}

		return true;
	}

	bool IsAdjacentBoxRangeCombinable(const VoxelInt3 &rangeBegin, const VoxelInt3 &rangeEnd, const VoxelInt3 &direction,
		Span3D<const VoxelBoxCombineResultID> entries, const VoxelChunk &voxelChunk)
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
					if (!IsAdjacentBoxCombinable(voxel, direction, entries, voxelChunk))
					{
						isCombinable = false;
						break;
					}
				}
			}
		}

		return isCombinable;
	}

	constexpr VoxelInt3 BoxCombineDirections[] =
	{
		VoxelInt3(1, 0, 0),
		VoxelInt3(0, 1, 0),
		VoxelInt3(0, 0, 1)
	};
}

VoxelBoxCombineResult::VoxelBoxCombineResult()
{
	this->clear();
}

void VoxelBoxCombineResult::clear()
{
	this->min = VoxelInt3::Zero;
	this->max = VoxelInt3::Zero;
}

void VoxelBoxCombineChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->entryIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->entryIDs.fill(-1);

	this->dirtyEntries.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->dirtyEntries.fill(false);
}

void VoxelBoxCombineChunk::update(Span<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk)
{
	this->dirtyEntryPositions.clear();
	this->dirtyEntryPositions.reserve(dirtyVoxels.getCount());

	// Free any combined boxes associated with the dirty voxels.
	for (const VoxelInt3 voxel : dirtyVoxels)
	{
		this->dirtyEntries.set(voxel.x, voxel.y, voxel.z, true);
		this->dirtyEntryPositions.emplace_back(voxel);

		const VoxelBoxCombineResultID entryID = this->entryIDs.get(voxel.x, voxel.y, voxel.z);
		if (entryID >= 0)
		{
			VoxelBoxCombineResult &boxCombineResult = this->combinedBoxesPool.get(entryID);

			for (WEInt currentZ = boxCombineResult.min.z; currentZ <= boxCombineResult.max.z; currentZ++)
			{
				for (int currentY = boxCombineResult.min.y; currentY <= boxCombineResult.max.y; currentY++)
				{
					for (SNInt currentX = boxCombineResult.min.x; currentX <= boxCombineResult.max.x; currentX++)
					{
						VoxelBoxCombineResultID &currentEntryID = this->entryIDs.get(currentX, currentY, currentZ);
						if (currentEntryID == entryID)
						{
							currentEntryID = -1;

							const VoxelInt3 currentVoxel(currentX, currentY, currentZ);
							this->dirtyEntries.set(currentX, currentY, currentZ, true);
							this->dirtyEntryPositions.emplace_back(currentVoxel); // Possible duplicate of 'this' voxel, sort + remove afterwards.
						}
					}
				}
			}

			this->combinedBoxesPool.free(entryID);
		}
	}

	// Sort dirty positions lexicographically to remove duplicates.
	std::sort(this->dirtyEntryPositions.begin(), this->dirtyEntryPositions.end(),
		[](const VoxelInt3 a, const VoxelInt3 b)
	{
		if (a.x != b.x)
		{
			return a.x < b.x;
		}
		else if (a.y != b.y)
		{
			return a.y < b.y;
		}
		else
		{
			return a.z < b.z;
		}
	});

	const auto uniqueDirtyPositionsEnd = std::unique(this->dirtyEntryPositions.begin(), this->dirtyEntryPositions.end());

	// Now sort by distance to origin so the combining algorithm has the best chance to generate big boxes
	// since it works along positive axes.
	std::sort(this->dirtyEntryPositions.begin(), uniqueDirtyPositionsEnd,
		[](const VoxelInt3 a, const VoxelInt3 b)
	{
		const int aLengthSqr = (a.x * a.x) + (a.y * a.y) + (a.z * a.z);
		const int bLengthSqr = (b.x * b.x) + (b.y * b.y) + (b.z * b.z);
		return aLengthSqr < bLengthSqr;
	});

	const int uniqueDirtyPositionCount = static_cast<int>(std::distance(this->dirtyEntryPositions.begin(), uniqueDirtyPositionsEnd));

	// Combine dirty boxes together where possible.
	for (int i = 0; i < uniqueDirtyPositionCount; i++)
	{
		const VoxelInt3 voxel = this->dirtyEntryPositions[i];
		bool &isBoxDirty = this->dirtyEntries.get(voxel.x, voxel.y, voxel.z);
		if (!isBoxDirty)
		{
			continue;
		}

		const VoxelShapeDefID shapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.shapeDefs[shapeDefID];
		const VoxelMeshDefinition &meshDef = shapeDef.mesh;
		if (meshDef.isEmpty())
		{
			bool isTriggerVoxel = false;
			VoxelTriggerDefID triggerDefID;
			if (voxelChunk.tryGetTriggerDefID(voxel.x, voxel.y, voxel.z, &triggerDefID))
			{
				const VoxelTriggerDefinition &voxelTriggerDef = voxelChunk.triggerDefs[triggerDefID];
				isTriggerVoxel = voxelTriggerDef.hasValidDefForPhysics();
			}

			if (!isTriggerVoxel)
			{
				// This voxel is air and has no trigger.
				isBoxDirty = false;
				continue;
			}
		}

		const VoxelBoxCombineResultID boxCombineResultID = this->combinedBoxesPool.alloc();
		if (boxCombineResultID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate voxel box combine result ID (voxel %s).", voxel.toString().c_str());
			continue;
		}

		VoxelBoxCombineResultID &entryID = this->entryIDs.get(voxel.x, voxel.y, voxel.z);
		DebugAssert(entryID == -1);
		entryID = boxCombineResultID;

		VoxelBoxCombineResult &faceCombineResult = this->combinedBoxesPool.get(boxCombineResultID);
		faceCombineResult.min = voxel;
		faceCombineResult.max = voxel;

		if (!shapeDef.allowsAdjacentFaceCombining)
		{
			// This voxel can't combine with anything.
			isBoxDirty = false;
			continue;
		}

		for (int combineDirectionIndex = 0; combineDirectionIndex < static_cast<int>(std::size(BoxCombineDirections)); combineDirectionIndex++)
		{
			const VoxelInt3 combineDirection = BoxCombineDirections[combineDirectionIndex];
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

				// Set all boxes in this range to be part of the combined box.
				for (WEInt combinedBoxZ = rangeBegin.z; combinedBoxZ <= rangeEnd.z; combinedBoxZ++)
				{
					for (int combinedBoxY = rangeBegin.y; combinedBoxY <= rangeEnd.y; combinedBoxY++)
					{
						for (SNInt combinedBoxX = rangeBegin.x; combinedBoxX <= rangeEnd.x; combinedBoxX++)
						{
							this->entryIDs.set(combinedBoxX, combinedBoxY, combinedBoxZ, boxCombineResultID);
							this->dirtyEntries.set(combinedBoxX, combinedBoxY, combinedBoxZ, false);
						}
					}
				}

				if (!IsAdjacentBoxRangeCombinable(rangeBegin, rangeEnd, combineDirection, this->entryIDs, voxelChunk))
				{
					// One or more voxels in the adjacent range aren't able to combine.
					break;
				}

				faceCombineResult.max = faceCombineResult.max + combineDirection;
			}
		}
	}
}

void VoxelBoxCombineChunk::clear()
{
	Chunk::clear();
	this->dirtyEntries.clear();
	this->dirtyEntryPositions.clear();
	this->combinedBoxesPool.clear();
	this->entryIDs.clear();
}
