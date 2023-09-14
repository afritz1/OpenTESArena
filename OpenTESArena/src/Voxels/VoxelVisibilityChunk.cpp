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
		const int levelNodeOffset = NODE_OFFSETS[i];
		const int levelNodeCount = NODE_COUNTS[i];
		const int levelNodesPerSide = NODES_PER_SIDE[i];
		const SNInt xDistPerNode = Chunk::WIDTH / levelNodesPerSide;
		const WEInt zDistPerNode = Chunk::DEPTH / levelNodesPerSide;

		for (WEInt z = 0; z < levelNodesPerSide; z++)
		{
			for (SNInt x = 0; x < levelNodesPerSide; x++)
			{
				const int bboxIndex = levelNodeOffset + (x + (z * levelNodesPerSide));
				DebugAssert(bboxIndex < (levelNodeOffset + levelNodeCount));
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

	// @todo: need a way to know if the current tree level needs further splitting or if it can stop
	// - maybe move the for loop guts to another updateTreeLevel(int) that returns whether it is done splitting.
	
	// Update all the visibility types and frustum tests so they can be read by draw call generation code later.
	for (int treeLevelIndex = 0; treeLevelIndex < TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		const int levelNodeOffset = NODE_OFFSETS[treeLevelIndex];
		const int levelNodeCount = NODE_COUNTS[treeLevelIndex];
		const int levelNodesPerSide = NODES_PER_SIDE[treeLevelIndex];
		const bool levelHasChildNodes = CHILD_COUNTS[treeLevelIndex] > 0;

		for (WEInt z = 0; z < levelNodesPerSide; z++)
		{
			for (SNInt x = 0; x < levelNodesPerSide; x++)
			{
				const int curNodeIndex = x + (z * levelNodesPerSide);
				const int bboxIndex = levelNodeOffset + curNodeIndex;
				DebugAssert(bboxIndex < (levelNodeOffset + levelNodeCount));
				DebugAssertIndex(this->nodeBBoxes, bboxIndex);
				const BoundingBox3D &bbox = this->nodeBBoxes[bboxIndex];

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

				if (levelHasChildNodes)
				{
					// Test bounding box against frustum planes.
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

					const int visibilityTypeIndex = bboxIndex;
					DebugAssertIndex(this->internalNodeVisibilityTypes, visibilityTypeIndex);

					if (isBBoxCompletelyVisible)
					{
						this->internalNodeVisibilityTypes[visibilityTypeIndex] = VisibilityType::Inside;

						// @todo: need another for loop in here that sets all this node's child visibility types and frustum test bools
					}
					else if (isBBoxCompletelyInvisible)
					{
						this->internalNodeVisibilityTypes[visibilityTypeIndex] = VisibilityType::Outside;

						// @todo: need another for loop in here that sets all this node's child visibility types and frustum test bools
					}
					else
					{
						this->internalNodeVisibilityTypes[visibilityTypeIndex] = VisibilityType::Partial;

						// @todo: tell the tree level index loop to continue for this node
						// - hmm, it's like a recursive step. Should be able to do iterative with all these index values though.

						// @todo: would be nice to be able to get the four child indices of a node, with -1 for leaf nodes' children
					}
				}
				else
				{
					// Populate frustum test bool since it's a leaf node.
					bool isVoxelColumnVisible = false;

					//DebugNotImplemented();

					const int leafNodeIndex = curNodeIndex;
					DebugAssertIndex(this->leafNodeFrustumTests, leafNodeIndex);
					this->leafNodeFrustumTests[leafNodeIndex] = isVoxelColumnVisible;
				}
			}
		}

		// @todo: if all nodes on this tree level are done testing, break.
		// - they are done if none are Partial visibility
	}
}

void VoxelVisibilityChunk::clear()
{
	Chunk::clear();
	std::fill(std::begin(this->nodeBBoxes), std::end(this->nodeBBoxes), BoundingBox3D());
	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}
