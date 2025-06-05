#ifndef VOXEL_FRUSTUM_CULLING_CHUNK_H
#define VOXEL_FRUSTUM_CULLING_CHUNK_H

#include "../Math/BoundingBox.h"
#include "../Rendering/VisibilityType.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

struct RenderCamera;

// Implements a quadtree for fast visibility tests against the camera.
struct VoxelFrustumCullingChunk final : public Chunk
{
	static constexpr int NODE_COUNT_LEVEL0 = 1; // Entire chunk.
	static constexpr int NODE_COUNT_LEVEL1 = 4;
	static constexpr int NODE_COUNT_LEVEL2 = 16;
	static constexpr int NODE_COUNT_LEVEL3 = 64;
	static constexpr int NODE_COUNT_LEVEL4 = 256;
	static constexpr int NODE_COUNT_LEVEL5 = 1024;
	static constexpr int NODE_COUNT_LEVEL6 = 4096; // Each voxel column.

	static constexpr int NODE_COUNTS[] =
	{
		NODE_COUNT_LEVEL0, NODE_COUNT_LEVEL1, NODE_COUNT_LEVEL2, NODE_COUNT_LEVEL3, NODE_COUNT_LEVEL4, NODE_COUNT_LEVEL5, NODE_COUNT_LEVEL6
	};

	static constexpr int TOTAL_NODE_COUNT = NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1 + NODE_COUNT_LEVEL2 + NODE_COUNT_LEVEL3 + NODE_COUNT_LEVEL4 + NODE_COUNT_LEVEL5 + NODE_COUNT_LEVEL6;
	static constexpr int LEAF_NODE_COUNT = NODE_COUNT_LEVEL6;
	static constexpr int INTERNAL_NODE_COUNT = TOTAL_NODE_COUNT - LEAF_NODE_COUNT;
	
	static constexpr int TREE_LEVEL_COUNT = static_cast<int>(std::size(NODE_COUNTS));
	static constexpr int INTERIOR_LEVEL_COUNT = TREE_LEVEL_COUNT - 1;
	static_assert(NODE_COUNTS[0] == 1);
	static_assert(NODE_COUNTS[TREE_LEVEL_COUNT - 1] == (Chunk::WIDTH * Chunk::DEPTH));

	static constexpr int TREE_LEVEL_INDEX_ROOT = 0;
	static constexpr int TREE_LEVEL_INDEX_LEAF = TREE_LEVEL_COUNT - 1;
	static_assert(TREE_LEVEL_INDEX_LEAF > TREE_LEVEL_INDEX_ROOT);

	static constexpr int GLOBAL_NODE_OFFSETS[] =
	{
		0,
		NODE_COUNT_LEVEL0,
		NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1,
		NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1 + NODE_COUNT_LEVEL2,
		NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1 + NODE_COUNT_LEVEL2 + NODE_COUNT_LEVEL3,
		NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1 + NODE_COUNT_LEVEL2 + NODE_COUNT_LEVEL3 + NODE_COUNT_LEVEL4,
		NODE_COUNT_LEVEL0 + NODE_COUNT_LEVEL1 + NODE_COUNT_LEVEL2 + NODE_COUNT_LEVEL3 + NODE_COUNT_LEVEL4 + NODE_COUNT_LEVEL5
	};

	static constexpr int NODES_PER_SIDE[] =
	{
		1, 2, 4, 8, 16, 32, 64
	};

	static constexpr int CHILD_COUNT_PER_NODE = 4;

	static constexpr int CHILD_COUNT_LEVEL0 = NODE_COUNT_LEVEL1;
	static constexpr int CHILD_COUNT_LEVEL1 = NODE_COUNT_LEVEL2;
	static constexpr int CHILD_COUNT_LEVEL2 = NODE_COUNT_LEVEL3;
	static constexpr int CHILD_COUNT_LEVEL3 = NODE_COUNT_LEVEL4;
	static constexpr int CHILD_COUNT_LEVEL4 = NODE_COUNT_LEVEL5;
	static constexpr int CHILD_COUNT_LEVEL5 = NODE_COUNT_LEVEL6;
	static constexpr int CHILD_COUNT_LEVEL6 = 0;

	static constexpr int CHILD_COUNTS[] =
	{
		CHILD_COUNT_LEVEL0, CHILD_COUNT_LEVEL1, CHILD_COUNT_LEVEL2, CHILD_COUNT_LEVEL3, CHILD_COUNT_LEVEL4, CHILD_COUNT_LEVEL5, CHILD_COUNT_LEVEL6
	};

	static constexpr int TOTAL_CHILD_COUNT = CHILD_COUNT_LEVEL0 + CHILD_COUNT_LEVEL1 + CHILD_COUNT_LEVEL2 + CHILD_COUNT_LEVEL3 + CHILD_COUNT_LEVEL4 + CHILD_COUNT_LEVEL5 + CHILD_COUNT_LEVEL6;

	// Various quadtree values populated in breadth-first order.
	BoundingBox3D nodeBBoxes[TOTAL_NODE_COUNT]; // Bounding boxes for each quadtree level. All have the same Y size.
	VisibilityType internalNodeVisibilityTypes[INTERNAL_NODE_COUNT]; // Non-leaf quadtree bbox tests against the camera this frame.
	bool leafNodeFrustumTests[LEAF_NODE_COUNT];

	VoxelFrustumCullingChunk();

	void init(const ChunkInt2 &position, int height, double ceilingScale);

	// Visibility of this chunk's fully-enclosing bounding box based on most recent frustum test.
	// If result is fully inside or outside, then all child nodes match that.
	VisibilityType getRootVisibilityType() const;

	void update(const RenderCamera &camera);
	void clear();
};

#endif
