#ifndef VOXEL_FACE_ENABLE_CHUNK_H
#define VOXEL_FACE_ENABLE_CHUNK_H

#include "VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

struct VoxelChunk;

struct VoxelFaceEnableEntry
{
	// For each face, should it be presented by the renderer?
	bool enabledFaces[VoxelUtils::FACE_COUNT];

	VoxelFaceEnableEntry();

	void fill(bool enabled);
};

struct VoxelFaceEnableChunk final : public Chunk
{
	Buffer3D<VoxelFaceEnableEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
