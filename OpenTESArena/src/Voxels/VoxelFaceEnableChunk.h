#ifndef VOXEL_FACE_ENABLE_CHUNK_H
#define VOXEL_FACE_ENABLE_CHUNK_H

#include "VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

class VoxelChunk;

struct VoxelFaceEnableEntry
{
	// For each face, should it be presented by the renderer?
	bool enabledFaces[VoxelUtils::FACE_COUNT];

	VoxelFaceEnableEntry();

	void fill(bool enabled);
};

class VoxelFaceEnableChunk final : public Chunk
{
public:
	Buffer3D<VoxelFaceEnableEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
