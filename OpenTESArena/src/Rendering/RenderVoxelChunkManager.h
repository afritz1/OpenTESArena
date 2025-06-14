#ifndef RENDER_VOXEL_CHUNK_MANAGER_H
#define RENDER_VOXEL_CHUNK_MANAGER_H

#include <array>
#include <optional>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderShaderUtils.h"
#include "RenderTransform.h"
#include "RenderVoxelChunk.h"
#include "../Voxels/VoxelChasmDefinition.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView3D.h"

class Renderer;
class RenderLightChunkManager;
class TextureManager;
class VoxelChunkManager;
class VoxelFaceCombineChunkManager;
class VoxelFrustumCullingChunkManager;

struct RenderCamera;
struct RenderCommandBuffer;
struct VoxelFrustumCullingChunk;

struct RenderVoxelLoadedTexture
{
	TextureAsset textureAsset;
	ScopedObjectTextureRef objectTextureRef;

	void init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
};

// Chasm walls are multi-textured. They use the chasm animation and a separate wall texture.
// The draw call and pixel shader need two textures in order to support chasm wall rendering.
struct RenderVoxelLoadedChasmFloorTexture
{
	VoxelChasmAnimationType animType;

	uint8_t paletteIndex;
	std::vector<TextureAsset> textureAssets; // All frames packed vertically into object texture ref.

	ScopedObjectTextureRef objectTextureRef;

	RenderVoxelLoadedChasmFloorTexture();

	void initColor(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
	void initTextured(std::vector<TextureAsset> &&textureAssets, ScopedObjectTextureRef &&objectTextureRef);
};

struct RenderVoxelLoadedChasmTextureKey
{
	VoxelChasmDefID chasmDefID;
	int chasmFloorListIndex;
	int chasmWallIndex; // Points into voxel textures.

	void init(VoxelChasmDefID chasmDefID, int chasmFloorListIndex, int chasmWallIndex);
};

// Allocated vertex buffer in model space, reusable at the same voxel span in any chunk (transformed by the world space uniform in that chunk).
struct RenderVoxelCombinedFaceVertexBuffer
{
	int voxelWidth, voxelHeight;
	VoxelShapeScaleType scaleType;
	VoxelFacing3D facing;

	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID;
	VertexAttributeBufferID texCoordBufferID;

	RenderVoxelCombinedFaceVertexBuffer();
};

class RenderVoxelChunkManager final : public SpecializedChunkManager<RenderVoxelChunk>
{
private:
	// Buffer for all raising doors' translation to push/pop during vertex shading so they scale towards the ceiling.
	// Updated on scene change.
	UniformBufferID raisingDoorPreScaleTranslationBufferID;

	// For combined voxel faces.
	IndexBufferID defaultQuadIndexBufferID;

	// Chasm wall support - one index buffer for each face combination.
	std::array<IndexBufferID, ArenaMeshUtils::CHASM_WALL_COMBINATION_COUNT> chasmWallIndexBufferIDs;

	std::vector<RenderVoxelLoadedTexture> textures; // Includes chasm walls.
	std::vector<RenderVoxelLoadedChasmFloorTexture> chasmFloorTextures;
	std::vector<RenderVoxelLoadedChasmTextureKey> chasmTextureKeys; // Points into floor lists and wall textures.

	// For reusing model space vertex buffers between chunks.
	std::vector<RenderVoxelCombinedFaceVertexBuffer> combinedFaceVertexBuffers;

	// All accumulated draw calls from scene components each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;

	ObjectTextureID getTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getChasmFloorTextureID(VoxelChasmDefID chasmDefID) const;
	ObjectTextureID getChasmWallTextureID(VoxelChasmDefID chasmDefID) const;

	void loadChunkTextures(const VoxelChunk &voxelChunk, const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager, Renderer &renderer);
	void loadMeshBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);
	void loadChasmWall(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z);
	void loadChasmWalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk);
	void loadTransforms(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);

	void updateChunkVoxelDrawCalls(RenderVoxelChunk &renderChunk, BufferView<const VoxelInt3> dirtyVoxelPositions, const VoxelChunk &voxelChunk,
		const RenderLightChunk &renderLightChunk, const VoxelChunkManager &voxelChunkManager, double ceilingScale, double chasmAnimPercent);
	void updateChunkCombinedVoxelDrawCalls(RenderVoxelChunk &renderChunk, BufferView<const VoxelInt3> dirtyVoxelPositions, const VoxelChunk &voxelChunk,
		const VoxelFaceCombineChunk &faceCombineChunk, const RenderLightChunk &renderLightChunk, const VoxelChunkManager &voxelChunkManager,
		double ceilingScale, double chasmAnimPercent, Renderer &renderer);

	void rebuildDrawCallsList(const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager);
public:
	RenderVoxelChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandBuffer(RenderCommandBuffer &commandBuffer) const;

	// Chunk allocating/freeing update function, called before voxel resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager, const VoxelFaceCombineChunkManager &voxelFaceCombineChunkManager,
		const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager, const RenderLightChunkManager &renderLightChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
