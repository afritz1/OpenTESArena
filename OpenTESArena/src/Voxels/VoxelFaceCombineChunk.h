#ifndef VOXEL_FACE_COMBINE_CHUNK_H
#define VOXEL_FACE_COMBINE_CHUNK_H

#include <vector>

#include "../Assets/TextureAsset.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

class VoxelChunk;

enum class PixelShaderType;
enum class RenderLightingType;
enum class VertexShaderType;

struct VoxelFacesEntry
{
	static constexpr int FACE_COUNT = 6;

	// +X, -X, +Y, -Y, +Z, -Z
	int combinedFacesIndices[FACE_COUNT];

	VoxelFacesEntry();

	void clear();
};

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

class VoxelFaceCombineChunk : public Chunk
{
public:
	std::vector<VoxelFaceCombineResult> combinedFaces;
	Buffer3D<VoxelFacesEntry> entries;

	void init(const ChunkInt2 &position, int height);

	void update(const BufferView<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
