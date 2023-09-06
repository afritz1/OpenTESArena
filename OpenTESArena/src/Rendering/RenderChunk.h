#ifndef RENDER_CHUNK_H
#define RENDER_CHUNK_H

#include <unordered_map>

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderVoxelMeshDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"

class Renderer;

using RenderVoxelMeshDefID = int;

// Lights relevant to a particular voxel. Each light might be touching several voxels.
struct RenderVoxelLightIdList
{
	RenderLightID lightIDs[RenderDrawCall::MAX_LIGHTS];
	int lightCount;

	RenderVoxelLightIdList();

	BufferView<RenderLightID> getLightIDs();
	BufferView<const RenderLightID> getLightIDs() const;
	void tryAddLight(RenderLightID id);
	void clear();
};

class RenderChunk final : public Chunk
{
public:
	static constexpr RenderVoxelMeshDefID AIR_MESH_DEF_ID = 0;

	std::vector<RenderVoxelMeshDefinition> meshDefs;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, RenderVoxelMeshDefID> meshDefMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<RenderVoxelMeshDefID> meshDefIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDsMap; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the render chunk manager.
	std::unordered_map<VoxelInt3, UniformBufferID> doorTransformBuffers; // Unique transform buffer per door instance, owned by this chunk.
	Buffer3D<RenderVoxelLightIdList> voxelLightIdLists; // Lights touching each voxel. IDs are owned by RenderChunkManager.
	std::vector<VoxelInt3> dirtyLightPositions; // Voxels that need relevant lights updated.
	std::vector<RenderDrawCall> staticDrawCalls; // Most voxel geometry (walls, floors, etc.).
	std::vector<RenderDrawCall> doorDrawCalls; // All doors, open or closed.
	std::vector<RenderDrawCall> chasmDrawCalls; // Chasm walls and floors, separate from static draw calls so their textures can animate.
	std::vector<RenderDrawCall> fadingDrawCalls; // Voxels with fade shader. Note that the static draw call in the same voxel needs to be deleted to avoid a conflict in the depth buffer.
	std::vector<RenderDrawCall> entityDrawCalls;

	void init(const ChunkInt2 &position, int height);
	RenderVoxelMeshDefID addMeshDefinition(RenderVoxelMeshDefinition &&meshDef);
	void addDirtyLightPosition(const VoxelInt3 &position);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
