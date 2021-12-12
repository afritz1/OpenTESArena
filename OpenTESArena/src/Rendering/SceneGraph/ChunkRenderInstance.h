#ifndef CHUNK_RENDER_INSTANCE_H
#define CHUNK_RENDER_INSTANCE_H

#include <vector>

#include "VoxelRenderInstance.h"

class ChunkRenderInstance
{
private:
	std::vector<VoxelRenderInstance> voxelRenderInsts;
	int defID; // Chunk render definition ID.
public:
	void init(int defID);

	int getDefID() const;

	int getVoxelRenderInstanceCount() const;
	const VoxelRenderInstance &getVoxelRenderInstance(int index) const;

	void addVoxelRenderInstance(VoxelRenderInstance &&inst);
	void clear();
};

#endif
