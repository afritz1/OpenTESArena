#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <array>
#include <optional>
#include <vector>

#include "SceneGraphChunk.h"
#include "../RenderDrawCall.h"
#include "../../Entities/CitizenUtils.h"

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

class ChunkManager;
class EntityDefinitionLibrary;
class EntityManager;
class LevelInstance;
class MapDefinition;
class Renderer;
class SkyInstance;
class TextureManager;

struct RenderCamera;

class SceneGraph
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

	using LoadedChasmWallTexture = LoadedVoxelTexture;

	struct LoadedChasmTextureKey
	{
		ChunkInt2 chunkPos;
		VoxelChunk::ChasmDefID chasmDefID;
		int chasmFloorListIndex;
		int chasmWallIndex;

		void init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, int chasmFloorListIndex, int chasmWallIndex);
	};
private:
	// Chunks with data for geometry storage, visibility calculation, etc..
	std::vector<SceneGraphChunk> graphChunks;
	
	// Chasm wall support - one index buffer for each face combination.
	std::array<IndexBufferID, ArenaMeshUtils::CHASM_WALL_COMBINATION_COUNT> chasmWallIndexBufferIDs;

	std::vector<LoadedVoxelTexture> voxelTextures;
	std::vector<LoadedChasmFloorTextureList> chasmFloorTextureLists;
	std::vector<LoadedChasmWallTexture> chasmWallTextures;
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

	std::optional<int> tryGetGraphChunkIndex(const ChunkInt2 &chunkPos) const;

	ObjectTextureID getVoxelTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const;

	void loadVoxelTextures(const VoxelChunk &chunk, TextureManager &textureManager, Renderer &renderer);
	void loadVoxelMeshBuffers(SceneGraphChunk &graphChunk, const VoxelChunk &chunk, double ceilingScale, RendererSystem3D &rendererSystem);
	void loadVoxelChasmWalls(SceneGraphChunk &graphChunk, const VoxelChunk &chunk);
	void loadVoxelDrawCalls(SceneGraphChunk &graphChunk, const VoxelChunk &chunk, double ceilingScale, double chasmAnimPercent);
public:
	void init(RendererSystem3D &rendererSystem);
	void shutdown(RendererSystem3D &rendererSystem);

	// Gets the list of draw calls for visible voxel geometry this frame.
	BufferView<const RenderDrawCall> getVoxelDrawCalls() const;

	// Loads all the resources required by the given voxel chunk and adds it to the draw list.
	// @todo: eventually I think a better way would be to simply treat scene graph chunks like allocated textures;
	// via function calls and operations on a returned handle/ID.
	void loadVoxelChunk(const VoxelChunk &chunk, double ceilingScale, TextureManager &textureManager, Renderer &renderer,
		RendererSystem3D &rendererSystem);
	
	// Call once per frame per chunk after all voxel chunk changes have been applied to the scene graph.
	// All context-sensitive data (like for chasm walls) should be available in the voxel chunk.
	void rebuildVoxelChunkDrawCalls(const VoxelChunk &voxelChunk, double ceilingScale, double chasmAnimPercent);

	void unloadVoxelChunk(const ChunkInt2 &chunkPos, RendererSystem3D &rendererSystem);

	// @todo: loadEntityChunk(), probably needs citizenGenInfo, nightLightsAreActive, playerHasLight, daytimePercent, entityDefLibrary
	// @todo: loadSky()
	// @todo: loadWeather()

	// Collects all stored voxel draw calls from active chunks and puts them into a list for the renderer.
	void rebuildVoxelDrawCallsList();

	// Clears all rendering resources from the scene graph (voxels, entities, sky, weather).
	void unloadScene(RendererSystem3D &rendererSystem);
};

#endif
