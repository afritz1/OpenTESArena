#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include <vector>

#include "VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/RecyclablePool.h"
#include "components/utilities/Span.h"

struct VoxelChunk;
struct VoxelFaceEnableChunk;

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

struct VoxelFaceCombineChunk final : public Chunk
{
private:
	Buffer3D<VoxelFaceCombineDirtyEntry> dirtyEntries;
	std::vector<VoxelInt3> dirtyEntryPositions; // Voxels that need dirty entry updating this frame. Cleared at start of each update.
public:
	RecyclablePool<VoxelFaceCombineResultID, VoxelFaceCombineResult> combinedFacesPool;
	Buffer3D<VoxelFacesEntry> entries;
	std::vector<VoxelFaceCombineResultID> dirtyIDs; // Combined faces freed this frame (potentially pointing to a different shape now) that dependent systems must refresh.

	void init(const ChunkInt2 &position, int height);

	void update(Span<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk, const VoxelFaceEnableChunk &faceEnableChunk);

	void clear();
};

#endif
