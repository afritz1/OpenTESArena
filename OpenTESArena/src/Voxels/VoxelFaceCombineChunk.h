#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include "VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class VoxelChunk;
class VoxelFaceEnableChunk;

// One or more adjacent voxel faces in the same plane combined into a quad.
struct VoxelFaceCombineResult
{
	VoxelInt3 min, max; // Inclusive max.
	VoxelFacing3D facing;

	VoxelFaceCombineResult();

	void clear();
};

using VoxelFaceCombineResultID = int;

// Mappings of voxel faces to their combined face ID if any.
struct VoxelFacesEntry
{
	VoxelFaceCombineResultID combinedFacesIDs[VoxelUtils::FACE_COUNT];

	VoxelFacesEntry();

	void clear();
};

// Faces marked for rebuilding this frame.
struct VoxelFaceCombineDirtyEntry
{
	bool dirtyFaces[VoxelUtils::FACE_COUNT];

	VoxelFaceCombineDirtyEntry();
};

class VoxelFaceCombineChunk : public Chunk
{
private:
	std::unordered_map<VoxelInt3, VoxelFaceCombineDirtyEntry> dirtyEntries; // Cleared at start of each update.
public:
	RecyclablePool<VoxelFaceCombineResultID, VoxelFaceCombineResult> combinedFacesPool;
	Buffer3D<VoxelFacesEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk, const VoxelFaceEnableChunk &faceEnableChunk);

	void clear();
};

#endif
