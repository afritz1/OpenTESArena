#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

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
		enum class Type
		{
			Color,
			Textured
		};

		Type type;

		uint8_t paletteIndex;
		std::vector<TextureAsset> textureAssets;

		std::vector<ScopedObjectTextureRef> objectTextureRefs; // If textured, then accessed via chasm anim percent.

		LoadedChasmFloorTextureList();

		void initColor(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
		void initTextured(std::vector<TextureAsset> &&textureAssets, std::vector<ScopedObjectTextureRef> &&objectTextureRefs);
	};

	using LoadedChasmWallTexture = LoadedVoxelTexture;

	struct LoadedChasmTextureKey
	{
		ChunkInt2 chunkPos;
		Chunk::ChasmID chasmID;
		int chasmFloorListIndex;
		int chasmWallIndex;

		void init(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID, int chasmFloorListIndex, int chasmWallIndex);
	};
private:
	// Chunks with data for geometry storage, visibility calculation, etc..
	std::vector<SceneGraphChunk> graphChunks;

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
	ObjectTextureID getChasmFloorTextureID(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID) const;

	void loadVoxelTextures(const Chunk &chunk, TextureManager &textureManager, Renderer &renderer);
	void loadVoxelMeshBuffers(SceneGraphChunk &graphChunk, const Chunk &chunk, const RenderCamera &camera,
		double ceilingScale, bool nightLightsAreActive, RendererSystem3D &renderer);
	void loadVoxelDrawCalls(SceneGraphChunk &graphChunk, const Chunk &chunk, double ceilingScale,
		double chasmAnimPercent);
public:
	// Gets the list of draw calls for visible geometry this frame.
	BufferView<const RenderDrawCall> getDrawCalls() const;

	// Loads all the rendering resources of the given scene into the scene graph.
	// @todo: eventually I think a better way would be to simply treat scene graph chunks like allocated textures;
	// via function calls and operations on a returned handle/ID.
	void loadScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
		double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
		double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
		TextureManager &textureManager, Renderer &renderer, RendererSystem3D &renderer3D);

	// Clears all rendering resources from the scene graph (voxels, entities, sky, weather).
	void unloadScene(RendererSystem3D &renderer);
};

#endif
