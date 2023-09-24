#include "VoxelVisibilityChunk.h"
#include "../Rendering/RenderCamera.h"

namespace
{
	// Child node indices for each internal node (root owns 4 indices, etc.) using breadth-first traversal.
	int CHILD_INDICES[VoxelVisibilityChunk::TOTAL_CHILD_COUNT];
	bool s_isChildIndicesInited = false;

	void InitChildIndexArray()
	{
		DebugAssert(!s_isChildIndicesInited);
		for (int i = 0; i < static_cast<int>(std::size(CHILD_INDICES)); i++)
		{
			CHILD_INDICES[i] = i + 1;
		}
	}

	// Quadtree visibility test state for a node that has to be pushed on the stack so its children nodes can be tested.
	struct SavedSubtreeTestState
	{
		int localNodeIndex; // One of the four child nodes.

		SavedSubtreeTestState()
		{
			this->clear();
		}

		void init(int localNodeIndex)
		{
			this->localNodeIndex = localNodeIndex;
		}

		void clear()
		{
			this->localNodeIndex = -1;
		}
	};

	// Converts the tree level + local node index to a global index for efficiently looking up a node in a chunk.
	int GetGlobalNodeIndex(int treeLevelIndex, int localNodeIndex)
	{
		DebugAssertIndex(VoxelVisibilityChunk::GLOBAL_NODE_OFFSETS, treeLevelIndex);
		return VoxelVisibilityChunk::GLOBAL_NODE_OFFSETS[treeLevelIndex] + (localNodeIndex * VoxelVisibilityChunk::CHILD_COUNT_PER_NODE);
	}

	// Gets the first of four child indices for a node on some tree level, or -1 if it's a leaf node.
	int GetFirstChildGlobalNodeIndex(int treeLevelIndex, int localNodeIndex)
	{
		DebugAssert(treeLevelIndex >= 0);
		DebugAssert(localNodeIndex >= 0);
		DebugAssert(localNodeIndex < VoxelVisibilityChunk::TOTAL_NODE_COUNT);

		if (treeLevelIndex == VoxelVisibilityChunk::TREE_LEVEL_INDEX_LEAF)
		{
			return -1;
		}

		return GetGlobalNodeIndex(treeLevelIndex + 1, localNodeIndex);
	}

	void BroadcastCompleteVisibilityResult(VoxelVisibilityChunk &chunk, int treeLevelIndex, int localNodeIndex, VisibilityType visibilityType)
	{
		static_assert(VoxelVisibilityChunk::CHILD_COUNT_PER_NODE == 4);
		DebugAssert(visibilityType != VisibilityType::Partial);

		const int nextTreeLevelIndex = treeLevelIndex + 1;
		const int firstChildIndex = GetGlobalNodeIndex(nextTreeLevelIndex, localNodeIndex);
		int childrenGlobalNodeIndices[VoxelVisibilityChunk::CHILD_COUNT_PER_NODE];
		childrenGlobalNodeIndices[0] = firstChildIndex;
		childrenGlobalNodeIndices[1] = firstChildIndex + 1;
		childrenGlobalNodeIndices[2] = firstChildIndex + 2;
		childrenGlobalNodeIndices[3] = firstChildIndex + 3;

		const bool treeLevelHasChildNodes = nextTreeLevelIndex < VoxelVisibilityChunk::TREE_LEVEL_INDEX_LEAF;
		if (treeLevelHasChildNodes)
		{
			for (const int childrenGlobalNodeIndex : childrenGlobalNodeIndices)
			{
				DebugAssertIndex(chunk.internalNodeVisibilityTypes, childrenGlobalNodeIndex);
				chunk.internalNodeVisibilityTypes[childrenGlobalNodeIndex] = visibilityType;
			}

			// @optimization: do this an iterative way instead
			BroadcastCompleteVisibilityResult(chunk, nextTreeLevelIndex, 0, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, nextTreeLevelIndex, 1, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, nextTreeLevelIndex, 2, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, nextTreeLevelIndex, 3, visibilityType);
		}
		else
		{
			const bool isAtLeastPartiallyVisible = visibilityType != VisibilityType::Outside;
			for (const int childrenGlobalNodeIndex : childrenGlobalNodeIndices)
			{
				const int childrenLocalNodeIndex = childrenGlobalNodeIndex - VoxelVisibilityChunk::INTERNAL_NODE_COUNT;
				DebugAssertIndex(chunk.leafNodeFrustumTests, childrenLocalNodeIndex);
				chunk.leafNodeFrustumTests[childrenLocalNodeIndex] = isAtLeastPartiallyVisible;
			}
		}
	}
}

VoxelVisibilityChunk::VoxelVisibilityChunk()
{
	if (!s_isChildIndicesInited)
	{
		InitChildIndexArray();
		s_isChildIndicesInited = true;
	}

	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}

void VoxelVisibilityChunk::init(const ChunkInt2 &position, int height, double ceilingScale)
{
	Chunk::init(position, height);

	const double yMax = static_cast<double>(height) * ceilingScale;
	const CoordDouble3 chunkMinCoord(position, VoxelDouble3::Zero);
	const CoordDouble3 chunkMaxCoord(position, VoxelDouble3(static_cast<SNDouble>(Chunk::WIDTH), yMax, static_cast<WEDouble>(Chunk::DEPTH)));
	const WorldDouble3 chunkMinPoint = VoxelUtils::coordToWorldPoint(chunkMinCoord);
	const WorldDouble3 chunkMaxPoint = VoxelUtils::coordToWorldPoint(chunkMaxCoord);

	// Initialize bounding boxes for each quadtree level.
	for (int i = 0; i < TREE_LEVEL_COUNT; i++)
	{
		// Each level covers the whole chunk but XZ iteration becomes more granular as level count increases.
		const int globalNodeOffset = GLOBAL_NODE_OFFSETS[i];
		const int levelNodeCount = NODE_COUNTS[i];
		const int levelNodesPerSide = NODES_PER_SIDE[i];
		const SNInt xDistPerNode = Chunk::WIDTH / levelNodesPerSide;
		const WEInt zDistPerNode = Chunk::DEPTH / levelNodesPerSide;

		for (WEInt z = 0; z < levelNodesPerSide; z++)
		{
			for (SNInt x = 0; x < levelNodesPerSide; x++)
			{
				const int bboxIndex = globalNodeOffset + (x + (z * levelNodesPerSide));
				DebugAssert(bboxIndex < (globalNodeOffset + levelNodeCount));
				DebugAssertIndex(this->nodeBBoxes, bboxIndex);
				BoundingBox3D &bbox = this->nodeBBoxes[bboxIndex];

				const SNDouble bboxMinX = static_cast<SNDouble>(x * xDistPerNode);
				const SNDouble bboxMaxX = static_cast<SNDouble>((x + 1) * xDistPerNode);
				const double bboxMinY = 0.0;
				const double bboxMaxY = yMax;
				const WEDouble bboxMinZ = static_cast<WEDouble>(z * zDistPerNode);
				const WEDouble bboxMaxZ = static_cast<WEDouble>((z + 1) * zDistPerNode);
				const WorldDouble3 bboxMinPoint = chunkMinPoint + WorldDouble3(bboxMinX, bboxMinY, bboxMinZ);
				const WorldDouble3 bboxMaxPoint = chunkMinPoint + WorldDouble3(bboxMaxX, bboxMaxY, bboxMaxZ);
				bbox.init(bboxMinPoint, bboxMaxPoint);
			}
		}
	}

	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}

void VoxelVisibilityChunk::update(const RenderCamera &camera)
{
	const WorldDouble3 cameraEye = camera.worldPoint;
	const Double3 frustumNormals[5] =
	{
		camera.forward, camera.leftFrustumNormal, camera.rightFrustumNormal, camera.bottomFrustumNormal, camera.topFrustumNormal
	};

	int currentTreeLevelIndex = 0;
	int currentLocalNodeIndex = 0;
	SavedSubtreeTestState savedSubtreeTestStates[TREE_LEVEL_COUNT - 1];
	int savedSubtreeTestStatesCount = 0;

	do
	{
		const int globalNodeIndex = GetGlobalNodeIndex(currentTreeLevelIndex, currentLocalNodeIndex);
		const BoundingBox3D &bbox = this->nodeBBoxes[globalNodeIndex];

		constexpr int bboxCornerCount = 8;
		const WorldDouble3 bboxCorners[bboxCornerCount] =
		{
			bbox.min,
			bbox.min + WorldDouble3(bbox.width, 0.0, 0.0),
			bbox.min + WorldDouble3(0.0, bbox.height, 0.0),
			bbox.min + WorldDouble3(bbox.width, bbox.height, 0.0),
			bbox.min + WorldDouble3(0.0, 0.0, bbox.depth),
			bbox.min + WorldDouble3(bbox.width, 0.0, bbox.depth),
			bbox.min + WorldDouble3(0.0, bbox.height, bbox.depth),
			bbox.max
		};

		bool isBBoxCompletelyVisible = true;
		bool isBBoxCompletelyInvisible = false;
		for (const Double3 &frustumNormal : frustumNormals)
		{
			int insidePoints = 0;
			int outsidePoints = 0;
			for (const WorldDouble3 &cornerPoint : bboxCorners)
			{
				const double dist = MathUtils::distanceToPlane(cornerPoint, cameraEye, frustumNormal);
				if (dist >= 0.0)
				{
					insidePoints++;
				}
				else
				{
					outsidePoints++;
				}
			}

			if (insidePoints < bboxCornerCount)
			{
				isBBoxCompletelyVisible = false;
			}

			if (outsidePoints == bboxCornerCount)
			{
				isBBoxCompletelyInvisible = true;
				break;
			}
		}

		const bool treeLevelHasChildNodes = currentTreeLevelIndex < TREE_LEVEL_INDEX_LEAF;
		if (treeLevelHasChildNodes)
		{
			VisibilityType visibilityType;
			if (isBBoxCompletelyInvisible)
			{
				visibilityType = VisibilityType::Outside;
			}
			else if (isBBoxCompletelyVisible)
			{
				visibilityType = VisibilityType::Inside;
			}
			else
			{
				visibilityType = VisibilityType::Partial;
			}

			DebugAssertIndex(this->internalNodeVisibilityTypes, globalNodeIndex);
			this->internalNodeVisibilityTypes[globalNodeIndex] = visibilityType;

			if (visibilityType == VisibilityType::Partial)
			{
				const int newSavedSubtreeTestStateIndex = savedSubtreeTestStatesCount;
				DebugAssertIndex(savedSubtreeTestStates, newSavedSubtreeTestStateIndex);
				savedSubtreeTestStates[newSavedSubtreeTestStateIndex].init(currentLocalNodeIndex);
				savedSubtreeTestStatesCount++;
				currentTreeLevelIndex++;
				currentLocalNodeIndex = 0;
				continue;
			}
			else
			{
				BroadcastCompleteVisibilityResult(*this, currentTreeLevelIndex, currentLocalNodeIndex, visibilityType);
			}
		}
		else
		{
			DebugAssertIndex(this->leafNodeFrustumTests, currentLocalNodeIndex);
			this->leafNodeFrustumTests[currentLocalNodeIndex] = !isBBoxCompletelyInvisible;
		}

		if (currentTreeLevelIndex == TREE_LEVEL_INDEX_ROOT)
		{
			break;
		}

		// Pop out of saved states, handling the case where it's the last node on a tree level.
		while ((currentLocalNodeIndex == (CHILD_COUNT_PER_NODE - 1)) && (savedSubtreeTestStatesCount > 0))
		{
			const int savedSubtreeTestStateIndex = savedSubtreeTestStatesCount - 1;
			SavedSubtreeTestState &savedSubtreeTestState = savedSubtreeTestStates[savedSubtreeTestStateIndex];
			currentLocalNodeIndex = savedSubtreeTestState.localNodeIndex;
			currentTreeLevelIndex--;
			savedSubtreeTestState.clear();
			savedSubtreeTestStatesCount--;
		}

		currentLocalNodeIndex++;
	} while (currentLocalNodeIndex < CHILD_COUNT_PER_NODE);
}

void VoxelVisibilityChunk::clear()
{
	Chunk::clear();
	std::fill(std::begin(this->nodeBBoxes), std::end(this->nodeBBoxes), BoundingBox3D());
	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}
