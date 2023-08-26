#include "VoxelVisibilityChunk.h"
#include "../Rendering/RenderCamera.h"

void VoxelVisibilityChunk::init(const ChunkInt2 &position, int height, double ceilingScale)
{
	Chunk::init(position, height);

	const CoordDouble3 chunkMinCoord(position, VoxelDouble3::Zero);
	const CoordDouble3 chunkMaxCoord(position, VoxelDouble3(static_cast<SNDouble>(Chunk::WIDTH), static_cast<double>(height) * ceilingScale, static_cast<WEDouble>(Chunk::DEPTH)));
	const WorldDouble3 chunkMinPoint = VoxelUtils::coordToWorldPoint(chunkMinCoord);
	const WorldDouble3 chunkMaxPoint = VoxelUtils::coordToWorldPoint(chunkMaxCoord);
	this->bbox.init(chunkMinPoint, chunkMaxPoint);

	this->insideFrustumTests.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->insideFrustumTests.fill(false);
}

void VoxelVisibilityChunk::update(const RenderCamera &camera)
{
	// @todo: test voxels for visibility

	this->insideFrustumTests.fill(true);
}

void VoxelVisibilityChunk::clear()
{
	Chunk::clear();
	this->bbox.clear();
	this->insideFrustumTests.clear();
}
