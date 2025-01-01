#ifndef RENDER_VOXEL_CHUNK_MANAGER_H
#define RENDER_VOXEL_CHUNK_MANAGER_H

#include <array>
#include <optional>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderShaderUtils.h"
#include "RenderTransform.h"
#include "RenderVoxelChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView3D.h"

class Renderer;
class RenderLightChunkManager;
class TextureManager;
class VoxelChunkManager;
class VoxelVisibilityChunkManager;

struct RenderCamera;
struct RenderCommandBuffer;
struct VoxelVisibilityChunk;

class RenderVoxelChunkManager final : public SpecializedChunkManager<RenderVoxelChunk>
{
public:
	struct LoadedTexture
	{
		TextureAsset textureAsset;
		ScopedObjectTextureRef objectTextureRef;

		void init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
	};

	// Chasm walls are multi-textured. They use the chasm animation and a separate wall texture.
	// The draw call and pixel shader need two textures in order to support chasm wall rendering.
	struct LoadedChasmFloorTextureList
	{
		VoxelChasmAnimationType animType;

		uint8_t paletteIndex;
		std::vector<TextureAsset> textureAssets;

		std::vector<ScopedObjectTextureRef> objectTextureRefs; // If textured, then accessed via chasm anim percent.

		LoadedChasmFloorTextureList();

		void initColor(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
		void initTextured(std::vector<TextureAsset> &&textureAssets, std::vector<ScopedObjectTextureRef> &&objectTextureRefs);

		int getTextureIndex(double chasmAnimPercent) const;
	};

	struct LoadedChasmTextureKey
	{
		ChunkInt2 chunkPos;
		VoxelChunk::ChasmDefID chasmDefID;
		int chasmFloorListIndex;
		int chasmWallIndex; // Points into voxel textures.

		void init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, int chasmFloorListIndex, int chasmWallIndex);
	};
private:
	// Buffer for all raising doors' translation to push/pop during vertex shading so they scale towards the ceiling.
	// Updated on scene change.
	UniformBufferID raisingDoorPreScaleTranslationBufferID;

	// Chasm wall support - one index buffer for each face combination.
	std::array<IndexBufferID, ArenaMeshUtils::CHASM_WALL_COMBINATION_COUNT> chasmWallIndexBufferIDs;

	std::vector<LoadedTexture> textures; // Includes chasm walls.
	std::vector<LoadedChasmFloorTextureList> chasmFloorTextureLists;
	std::vector<LoadedChasmTextureKey> chasmTextureKeys; // Points into floor lists and wall textures.

	// All accumulated draw calls from scene components each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;

	ObjectTextureID getTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const;

	void loadTextures(const VoxelChunk &voxelChunk, TextureManager &textureManager, Renderer &renderer);
	void loadMeshBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);
	void loadChasmWall(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z);
	void loadChasmWalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk);
	void loadTransforms(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);

	void updateChunkDrawCalls(RenderVoxelChunk &renderChunk, BufferView<const VoxelInt3> dirtyVoxelPositions, const VoxelChunk &voxelChunk,
		const RenderLightChunk &renderLightChunk, double ceilingScale, double chasmAnimPercent);

	void rebuildDrawCallsList(const VoxelVisibilityChunkManager &voxelVisChunkManager);
public:
	RenderVoxelChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandBuffer(RenderCommandBuffer &commandBuffer) const;

	// Chunk allocating/freeing update function, called before voxel resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager,
		const VoxelVisibilityChunkManager &voxelVisChunkManager, const RenderLightChunkManager &renderLightChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
