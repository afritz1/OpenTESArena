#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

class VoxelChunk;

struct VoxelFacesEntry
{
	static constexpr int FACE_COUNT = 6;

	// If >= 0, points into combined face instances.
	// +X, -X, +Y, -Y, +Z, -Z
	int faces[FACE_COUNT];

	VoxelFacesEntry();

	void clear();
};

class VoxelFaceCombineChunk : public Chunk
{
public:
	// @todo list of combined faces to make meshes out of
	Buffer3D<VoxelFacesEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(const BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
