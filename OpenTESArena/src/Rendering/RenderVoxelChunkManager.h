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

class Renderer;
class TextureManager;
class VoxelChunkManager;
class VoxelVisibilityChunkManager;

struct RenderCamera;
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
		ChasmDefinition::AnimationType animType;

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
	// Default identity transform for voxels that are only offset from the origin and nothing else.
	UniformBufferID defaultTransformBufferID;

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
	void loadDoorUniformBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);

	void addDrawCall(const Double3 &position, UniformBufferID transformBufferID, int transformIndex, VertexBufferID vertexBufferID,
		AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID, IndexBufferID indexBufferID, ObjectTextureID textureID0,
		const std::optional<ObjectTextureID> &textureID1, TextureSamplingType textureSamplingType0, TextureSamplingType textureSamplingType1,
		RenderLightingType lightingType, double meshLightPercent, BufferView<const RenderLightID> lightIDs, VertexShaderType vertexShaderType,
		PixelShaderType pixelShaderType, double pixelShaderParam0, std::vector<RenderDrawCall> &drawCalls);
	void loadDrawCalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, double chasmAnimPercent,
		bool updateStatics, bool updateAnimating);

	// Call once per frame per chunk after all voxel chunk changes have been applied to this manager.
	// All context-sensitive data (like for chasm walls) should be available in the voxel chunk.
	void rebuildChunkDrawCalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		double chasmAnimPercent, bool updateStatics, bool updateAnimating);
	void rebuildDrawCallsList();
public:
	RenderVoxelChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	BufferView<const RenderDrawCall> getDrawCalls() const;

	// Chunk allocating/freeing update function, called before voxel resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager,
		const VoxelVisibilityChunkManager &voxelVisChunkManager, TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
