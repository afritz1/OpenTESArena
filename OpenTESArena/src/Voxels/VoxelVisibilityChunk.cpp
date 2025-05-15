#include "VoxelVisibilityChunk.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"

#include "components/utilities/BufferView.h"

namespace
{
	// Child node indices for each internal node (root owns 4 indices, etc.) using breadth-first traversal.
	int CHILD_INDICES[VoxelVisibilityChunk::TOTAL_CHILD_COUNT];

	// XY point for each tree level node index.
	Int2 Z_ORDER_CURVE_POINTS_LEVEL0[VoxelVisibilityChunk::NODE_COUNT_LEVEL0];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL1[VoxelVisibilityChunk::NODE_COUNT_LEVEL1];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL2[VoxelVisibilityChunk::NODE_COUNT_LEVEL2];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL3[VoxelVisibilityChunk::NODE_COUNT_LEVEL3];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL4[VoxelVisibilityChunk::NODE_COUNT_LEVEL4];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL5[VoxelVisibilityChunk::NODE_COUNT_LEVEL5];
	Int2 Z_ORDER_CURVE_POINTS_LEVEL6[VoxelVisibilityChunk::NODE_COUNT_LEVEL6];
	BufferView<Int2> Z_ORDER_CURVE_POINT_ARRAYS[] =
	{
		Z_ORDER_CURVE_POINTS_LEVEL0,
		Z_ORDER_CURVE_POINTS_LEVEL1,
		Z_ORDER_CURVE_POINTS_LEVEL2,
		Z_ORDER_CURVE_POINTS_LEVEL3,
		Z_ORDER_CURVE_POINTS_LEVEL4,
		Z_ORDER_CURVE_POINTS_LEVEL5,
		Z_ORDER_CURVE_POINTS_LEVEL6
	};

	bool s_areGlobalVariablesInited = false;

	void InitChildIndexArray()
	{
		DebugAssert(!s_areGlobalVariablesInited);

		for (int i = 0; i < static_cast<int>(std::size(CHILD_INDICES)); i++)
		{
			CHILD_INDICES[i] = i + 1;
		}
	}

	void InitZOrderCurvePointsArrays()
	{
		DebugAssert(!s_areGlobalVariablesInited);

		for (BufferView<Int2> dstArray : Z_ORDER_CURVE_POINT_ARRAYS)
		{
			for (int i = 0; i < dstArray.getCount(); i++)
			{
				dstArray[i] = MathUtils::getZOrderCurvePoint(i);
			}
		}
	}

	// Quadtree visibility test state for a node that has to be pushed on the stack so its children nodes can be tested.
	struct SavedSubtreeTestState
	{
		int treeLevelNodeIndex; // 0-# of nodes on this tree level, points to one of the four child nodes.

		SavedSubtreeTestState()
		{
			this->clear();
		}

		void init(int treeLevelNodeIndex)
		{
			this->treeLevelNodeIndex = treeLevelNodeIndex;
		}

		void clear()
		{
			this->treeLevelNodeIndex = -1;
		}
	};

	// Converts a tree level index and tree level node index to a Z-order curve quadkey.
	// Each Z is four sequential node indices.
	int GetZOrderCurveNodeIndex(int treeLevelIndex, int treeLevelNodeIndex)
	{
		DebugAssertIndex(Z_ORDER_CURVE_POINT_ARRAYS, treeLevelIndex);
		const BufferView<Int2> zOrderCurvePoints = Z_ORDER_CURVE_POINT_ARRAYS[treeLevelIndex];
		const Int2 point = zOrderCurvePoints[treeLevelNodeIndex];

		DebugAssertIndex(VoxelVisibilityChunk::NODES_PER_SIDE, treeLevelIndex);
		const int nodesPerSide = VoxelVisibilityChunk::NODES_PER_SIDE[treeLevelIndex];
		return point.x + (point.y * nodesPerSide);
	}

	// Gets the first of four child indices one level down from an internal node.
	int GetFirstChildTreeLevelNodeIndex(int treeLevelNodeIndex)
	{
		return treeLevelNodeIndex * VoxelVisibilityChunk::CHILD_COUNT_PER_NODE;
	}

	// Converts the "0-# of nodes on tree level - 1" value to 0-3 for a specific subtree.
	int GetSubtreeChildNodeIndex(int treeLevelNodeIndex)
	{
		return treeLevelNodeIndex % VoxelVisibilityChunk::CHILD_COUNT_PER_NODE;
	}

	void BroadcastCompleteVisibilityResult(VoxelVisibilityChunk &chunk, int treeLevelIndex, int treeLevelNodeIndex, VisibilityType visibilityType)
	{
		static_assert(VoxelVisibilityChunk::CHILD_COUNT_PER_NODE == 4);
		DebugAssert(treeLevelIndex < VoxelVisibilityChunk::TREE_LEVEL_INDEX_LEAF);
		DebugAssert(visibilityType != VisibilityType::Partial);

		const bool isAtLeastPartiallyVisible = visibilityType != VisibilityType::Outside;

		// Very fast writes if the root node is completely visible/invisible.
		if (treeLevelIndex == VoxelVisibilityChunk::TREE_LEVEL_INDEX_ROOT)
		{
			std::fill(std::begin(chunk.internalNodeVisibilityTypes), std::end(chunk.internalNodeVisibilityTypes), visibilityType);
			std::fill(std::begin(chunk.leafNodeFrustumTests), std::end(chunk.leafNodeFrustumTests), isAtLeastPartiallyVisible);
			return;
		}

		const int firstChildTreeLevelNodeIndex = GetFirstChildTreeLevelNodeIndex(treeLevelNodeIndex);
		int childrenTreeLevelNodeIndices[VoxelVisibilityChunk::CHILD_COUNT_PER_NODE];
		childrenTreeLevelNodeIndices[0] = firstChildTreeLevelNodeIndex;
		childrenTreeLevelNodeIndices[1] = firstChildTreeLevelNodeIndex + 1;
		childrenTreeLevelNodeIndices[2] = firstChildTreeLevelNodeIndex + 2;
		childrenTreeLevelNodeIndices[3] = firstChildTreeLevelNodeIndex + 3;

		const int childrenTreeLevelIndex = treeLevelIndex + 1;
		const bool childrenTreeLevelHasChildNodes = childrenTreeLevelIndex < VoxelVisibilityChunk::TREE_LEVEL_INDEX_LEAF;
		if (childrenTreeLevelHasChildNodes)
		{
			DebugAssertIndex(VoxelVisibilityChunk::GLOBAL_NODE_OFFSETS, childrenTreeLevelIndex);
			const int globalNodeOffset = VoxelVisibilityChunk::GLOBAL_NODE_OFFSETS[childrenTreeLevelIndex];

			for (const int childTreeLevelNodeIndex : childrenTreeLevelNodeIndices)
			{
				const int zOrderCurveNodeIndex = GetZOrderCurveNodeIndex(childrenTreeLevelIndex, childTreeLevelNodeIndex);
				const int globalNodeIndex = globalNodeOffset + zOrderCurveNodeIndex;
				DebugAssertIndex(chunk.internalNodeVisibilityTypes, globalNodeIndex);
				chunk.internalNodeVisibilityTypes[globalNodeIndex] = visibilityType;
			}

			// @optimization: do this an iterative way instead
			BroadcastCompleteVisibilityResult(chunk, childrenTreeLevelIndex, firstChildTreeLevelNodeIndex, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, childrenTreeLevelIndex, firstChildTreeLevelNodeIndex + 1, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, childrenTreeLevelIndex, firstChildTreeLevelNodeIndex + 2, visibilityType);
			BroadcastCompleteVisibilityResult(chunk, childrenTreeLevelIndex, firstChildTreeLevelNodeIndex + 3, visibilityType);
		}
		else
		{
			for (const int childTreeLevelNodeIndex : childrenTreeLevelNodeIndices)
			{
				const int zOrderCurveNodeIndex = GetZOrderCurveNodeIndex(childrenTreeLevelIndex, childTreeLevelNodeIndex);
				DebugAssertIndex(chunk.leafNodeFrustumTests, zOrderCurveNodeIndex);
				chunk.leafNodeFrustumTests[zOrderCurveNodeIndex] = isAtLeastPartiallyVisible;
			}
		}
	}
}

VoxelVisibilityChunk::VoxelVisibilityChunk()
{
	if (!s_areGlobalVariablesInited)
	{
		InitChildIndexArray();
		InitZOrderCurvePointsArrays();
		s_areGlobalVariablesInited = true;
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
				const int globalNodeIndex = globalNodeOffset + (x + (z * levelNodesPerSide));
				DebugAssert(globalNodeIndex < (globalNodeOffset + levelNodeCount));
				DebugAssertIndex(this->nodeBBoxes, globalNodeIndex);
				BoundingBox3D &bbox = this->nodeBBoxes[globalNodeIndex];

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

bool VoxelVisibilityChunk::anyVisibleLeafNodes() const
{
	constexpr int rootNodeIndex = 0;
	return this->internalNodeVisibilityTypes[rootNodeIndex] != VisibilityType::Outside;
}

void VoxelVisibilityChunk::update(const RenderCamera &camera)
{
	int currentTreeLevelIndex = 0; // Starts at root, ends at leaves.
	int currentTreeLevelNodeIndex = 0; // 0-# of nodes on the current tree level.
	SavedSubtreeTestState savedSubtreeTestStates[TREE_LEVEL_COUNT - 1];
	int savedSubtreeTestStatesCount = 0;

	do
	{
		DebugAssertIndex(GLOBAL_NODE_OFFSETS, currentTreeLevelIndex);
		const int globalNodeOffset = GLOBAL_NODE_OFFSETS[currentTreeLevelIndex];
		
		const int zOrderCurveNodeIndex = GetZOrderCurveNodeIndex(currentTreeLevelIndex, currentTreeLevelNodeIndex);
		const int globalNodeIndex = globalNodeOffset + zOrderCurveNodeIndex;

		DebugAssertIndex(this->nodeBBoxes, globalNodeIndex);
		const BoundingBox3D &bbox = this->nodeBBoxes[globalNodeIndex];

		bool isBBoxCompletelyVisible, isBBoxCompletelyInvisible;
		RendererUtils::getBBoxVisibilityInFrustum(bbox, camera, &isBBoxCompletelyVisible, &isBBoxCompletelyInvisible);

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
				savedSubtreeTestStates[newSavedSubtreeTestStateIndex].init(currentTreeLevelNodeIndex);
				savedSubtreeTestStatesCount++;
				currentTreeLevelIndex++;
				currentTreeLevelNodeIndex = GetFirstChildTreeLevelNodeIndex(currentTreeLevelNodeIndex);
				continue;
			}
			else
			{
				BroadcastCompleteVisibilityResult(*this, currentTreeLevelIndex, currentTreeLevelNodeIndex, visibilityType);
			}
		}
		else
		{
			DebugAssertIndex(this->leafNodeFrustumTests, zOrderCurveNodeIndex);
			this->leafNodeFrustumTests[zOrderCurveNodeIndex] = !isBBoxCompletelyInvisible;
		}

		// Pop out of saved states, handling the case where it's the last node on a tree level.
		int currentSubtreeChildIndex = GetSubtreeChildNodeIndex(currentTreeLevelNodeIndex);
		while ((currentSubtreeChildIndex == (CHILD_COUNT_PER_NODE - 1)) && (savedSubtreeTestStatesCount > 0))
		{
			const int savedSubtreeTestStateIndex = savedSubtreeTestStatesCount - 1;
			SavedSubtreeTestState &savedSubtreeTestState = savedSubtreeTestStates[savedSubtreeTestStateIndex];
			currentTreeLevelNodeIndex = savedSubtreeTestState.treeLevelNodeIndex;
			currentSubtreeChildIndex = GetSubtreeChildNodeIndex(currentTreeLevelNodeIndex);
			currentTreeLevelIndex--;
			savedSubtreeTestState.clear();
			savedSubtreeTestStatesCount--;
		}

		if (currentTreeLevelIndex == TREE_LEVEL_INDEX_ROOT)
		{
			break;
		}

		currentTreeLevelNodeIndex++;
	} while (GetSubtreeChildNodeIndex(currentTreeLevelNodeIndex) < CHILD_COUNT_PER_NODE);
}

void VoxelVisibilityChunk::clear()
{
	Chunk::clear();
	std::fill(std::begin(this->nodeBBoxes), std::end(this->nodeBBoxes), BoundingBox3D());
	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}
