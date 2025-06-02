#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include "../Assets/TextureAsset.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class VoxelChunk;

enum class PixelShaderType;
enum class RenderLightingType;
enum class VertexShaderType;

struct VoxelFaceCombineResult
{
	VoxelInt3 min, max; // Inclusive max.
	VoxelFacing3D facing;
	TextureAsset textureAsset;
	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;
	RenderLightingType lightingType;

	VoxelFaceCombineResult();

	void clear();
};

using VoxelFaceCombineResultID = int;

struct VoxelFacesEntry
{
	static constexpr int FACE_COUNT = 6; // +X, -X, +Y, -Y, +Z, -Z

	VoxelFaceCombineResultID combinedFacesIDs[FACE_COUNT];

	VoxelFacesEntry();

	void clear();
};

class VoxelFaceCombineChunk : public Chunk
{
public:
	RecyclablePool<VoxelFaceCombineResult, VoxelFaceCombineResultID> combinedFacesPool;
	Buffer3D<VoxelFacesEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
