#ifndef SCENE_GRAPH_CHUNK_H
#define SCENE_GRAPH_CHUNK_H

#include "../RenderTriangle.h"
#include "../../World/VoxelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

struct SceneGraphVoxel
{
	Buffer<RenderTriangle> opaqueTriangles, alphaTestedTriangles;
};

struct SceneGraphChunk
{
	ChunkInt2 position;
	Buffer3D<SceneGraphVoxel> voxels;
	// @todo: quadtree
	// - thinking that each 'visible slice' of the tree could be a BufferView2D maybe, or a VoxelInt2 begin + end pattern

	void init(const ChunkInt2 &position, int height);
};

#endif
