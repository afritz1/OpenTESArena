#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

struct VoxelFacesEntry
{
	// If >= 0, points into combined face instances.
	// +X, -X, +Y, -Y, +Z, -Z
	int faces[6];

	VoxelFacesEntry();

	void clear();
};

class VoxelFaceCombineChunk : public Chunk
{
public:
	// @todo list of combined faces
	Buffer3D<VoxelFacesEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(const BufferView<const VoxelInt3> dirtyVoxels);

	void clear();
};

#endif
