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
	const ChunkInt2 &chunkPos = this->getPosition();
	const WorldDouble3 cameraEye = camera.worldPoint;
	const Double3 frustumNormals[4] =
	{
		camera.leftFrustumNormal, camera.rightFrustumNormal, camera.bottomFrustumNormal, camera.topFrustumNormal
	};

	DebugLogError("Not implemented: VoxelVisibilityChunk::update()");

	// @todo: do chunk test

	/*int nodeLevelIndex = 0; // Start with the whole chunk.
	while (nodeLevelIndex < TREE_LEVEL_COUNT)
	{


		nodeLevelIndex++;
	}*/

	//std::fill(std::begin(this->nonLeafVisibilityTypes), std::end(this->nonLeafVisibilityTypes), VisibilityType::Inside);

	// @todo: do quadtree tests
	//std::fill(std::begin(this->leafFrustumTests), std::end(this->leafFrustumTests), true);
	//DebugNotImplemented();
}

void VoxelVisibilityChunk::clear()
{
	Chunk::clear();
	std::fill(std::begin(this->nodeBBoxes), std::end(this->nodeBBoxes), BoundingBox3D());
	std::fill(std::begin(this->internalNodeVisibilityTypes), std::end(this->internalNodeVisibilityTypes), VisibilityType::Outside);
	std::fill(std::begin(this->leafNodeFrustumTests), std::end(this->leafNodeFrustumTests), false);
}
