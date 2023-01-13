#ifndef RENDER_CHUNK_MANAGER_H
#define RENDER_CHUNK_MANAGER_H

#include <array>
#include <optional>
#include <vector>

#include "RenderChunk.h"
#include "RenderDrawCall.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// Objectives:
// - efficient visible voxel determination (use quadtree per chunk to find ranges of visible voxel columns)
// - efficient visible entity determination (don't have notion of a 'chunk' for entities yet, might want bounding box hierarchy per chunk?)
// - efficient visible sky determination (simple dot product >0 against camera direction at first? or use +/- sign of each star direction? camera can specify which +/- octants are visible to it)

// Wishlist:
// - make groups of voxels/entities/sky batchable (mesh combining, texture atlasing, etc.)
// - batching/ordering draw list by texture
// - occlusion culling system or hierarchical Z buffer to reduce/eliminate overdraw
// - avoid requiring a screen clear

class EntityChunkManager;
class Renderer;
class TextureManager;
class VoxelChunkManager;

struct RenderCamera;

class RenderChunkManager final : public SpecializedChunkManager<RenderChunk>
{
public:
	struct LoadedVoxelTexture
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
	// Chasm wall support - one index buffer for each face combination.
	std::array<IndexBufferID, ArenaMeshUtils::CHASM_WALL_COMBINATION_COUNT> chasmWallIndexBufferIDs;

	std::vector<LoadedVoxelTexture> voxelTextures; // Includes chasm walls.
	std::vector<LoadedChasmFloorTextureList> chasmFloorTextureLists;
	std::vector<LoadedChasmTextureKey> chasmTextureKeys; // Points into floor lists and wall textures.

	// @todo: entity rendering resources

	// @todo: sky rendering resources
	// - hemisphere geometry w/ texture IDs and coordinates for colors (use some trig functions for vertex generation?)
	// - list of vertex buffer IDs
	// - list of transforms for sky positions
	// - order matters: stars, sun, planets, clouds, mountains (etc.)

	// @todo: weather particles
	// - list of texture IDs
	// - list of vertex buffer ids (all in 2D and in model space)
	// - list of transforms for screen positions

	// All accumulated draw calls from scene components each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;

	ObjectTextureID getVoxelTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const;

	void loadVoxelTextures(const VoxelChunk &voxelChunk, TextureManager &textureManager, Renderer &renderer);
	void loadVoxelMeshBuffers(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);
	void loadVoxelChasmWalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk);

	void addVoxelDrawCall(const Double3 &position, const Double3 &preScaleTranslation, const Matrix4d &rotationMatrix,
		const Matrix4d &scaleMatrix, VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
		IndexBufferID indexBufferID, ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1,
		TextureSamplingType textureSamplingType, VertexShaderType vertexShaderType, PixelShaderType pixelShaderType,
		double pixelShaderParam0, std::vector<RenderDrawCall> &drawCalls);
	void loadVoxelDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		double chasmAnimPercent, bool updateStatics, bool updateAnimating);

	// Loads all the resources required by the given voxel chunk and adds it to the draw list.
	// @todo: eventually I think a better way would be to simply treat render chunks like allocated textures;
	// via function calls and operations on a returned handle/ID.
	void populateVoxelChunk(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		TextureManager &textureManager, Renderer &renderer);

	// Call once per frame per chunk after all voxel chunk changes have been applied to this manager.
	// All context-sensitive data (like for chasm walls) should be available in the voxel chunk.
	void rebuildVoxelChunkDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		double chasmAnimPercent, bool updateStatics, bool updateAnimating);

	// @todo: loadSky()
	// @todo: loadWeather()

	// Collects all stored voxel draw calls from active chunks and puts them into a list for the renderer.
	void rebuildVoxelDrawCallsList();
public:
	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	// Gets the list of draw calls for visible voxel geometry this frame.
	BufferView<const RenderDrawCall> getVoxelDrawCalls() const;

	// Chunk allocating/freeing update function, called before voxel or entity resources are updated.
	void updateActiveChunks(const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void updateVoxels(const BufferView<const ChunkInt2> &activeChunkPositions, const BufferView<const ChunkInt2> &newChunkPositions,
		double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager,
		Renderer &renderer);
	void updateEntities(const BufferView<const ChunkInt2> &activeChunkPositions, const BufferView<const ChunkInt2> &newChunkPositions,
		const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);

	// Clears all rendering resources (voxels, entities, sky, weather).
	void unloadScene(Renderer &renderer);
};

#endif
