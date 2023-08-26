#ifndef VOXEL_VISIBILITY_CHUNK_H
#define VOXEL_VISIBILITY_CHUNK_H

#include "../Math/BoundingBox.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

struct RenderCamera;

struct VoxelVisibilityChunk final : public Chunk
{
	BoundingBox3D bbox; // Contains the entire chunk.
	// @todo: mipmapped bounding boxes for each quadtree level; all are the same Y size
	// - each bounding box should have a set of voxel indices it covers
	
	Buffer3D<bool> insideFrustumTests;
	
	// @todo: a "visibleToEyeTests" Buffer3D which would check occlusion too. Cares about ceilingScale-corrected corners and opaque faces. Projects into camera space?
	// - this could potentially get very complicated (i.e. each occlusion test could check a 3D tile of voxels instead of just one...)

	void init(const ChunkInt2 &position, int height, double ceilingScale);

	void update(const RenderCamera &camera);
	void clear();
};

#endif
