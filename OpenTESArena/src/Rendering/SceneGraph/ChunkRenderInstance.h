#ifndef CHUNK_RENDER_INSTANCE_H
#define CHUNK_RENDER_INSTANCE_H

#include <vector>

#include "VoxelRenderInstance.h"
#include "../../World/Coord.h"

class ChunkRenderInstance
{
private:
	// @todo: quadtree instance for keeping track of visible voxel columns for THIS chunk
	
	// @todo: bounding box that expands based on entities inserted into the scene graph that occupy this chunk
	// - guides all-or-nothing entity culling per chunk
	// - When inserting entity instances into the scene graph, we should know their chunk and individual bounding box so we can
	//   expand the total bounding box for all entities in the chunk. If the chunk's total bounding box is off camera, all entities
	//   are culled. We know by default that the chunk's bounding box is always at least its dimensions (64xHx64).

	std::vector<VoxelRenderInstance> voxelRenderInsts;
	ChunkInt2 coord;
	int defID; // Chunk render definition ID.
public:
	void init(int defID, const ChunkInt2 &coord);

	int getDefID() const;

	const ChunkInt2 &getCoord() const;

	int getVoxelRenderInstanceCount() const;
	const VoxelRenderInstance &getVoxelRenderInstance(int index) const;

	void addVoxelRenderInstance(VoxelRenderInstance &&inst);
	void clear();
};

#endif
