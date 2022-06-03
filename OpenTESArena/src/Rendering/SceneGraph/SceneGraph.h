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

	struct LoadedEntityTexture
	{
		TextureAsset textureAsset;
		bool flipped;
		bool reflective;
		ScopedObjectTextureRef objectTextureRef;

		void init(const TextureAsset &textureAsset, bool flipped, bool reflective,
			ScopedObjectTextureRef &&objectTextureRef);
	};

	struct LoadedChasmTextureList
	{
		// Chasm walls are multi-textured. They use the chasm animation and a separate wall texture.
		// The draw call and pixel shader need two textures in order to support chasm wall rendering.
		struct WallEntry
		{
			TextureAsset wallTextureAsset;
			ScopedObjectTextureRef wallTextureRef;
		};

		ArenaTypes::ChasmType chasmType;
		std::vector<ScopedObjectTextureRef> chasmTextureRefs;
		std::vector<WallEntry> wallEntries;

		void init(ArenaTypes::ChasmType chasmType);
	};
private:
	// Chunks with data for geometry storage, visibility calculation, etc..
	std::vector<SceneGraphChunk> graphChunks;

	std::vector<LoadedVoxelTexture> voxelTextures;
	std::vector<LoadedEntityTexture> entityTextures;
	std::vector<LoadedChasmTextureList> chasmTextureLists;

	// @todo: sky rendering resources
	// - hemisphere geometry w/ texture IDs and coordinates for colors (use some trig functions for vertex generation?)
	// - list of vertex buffer IDs
	// - list of transforms for sky positions
	// - order matters: stars, sun, planets, clouds, mountains (etc.)

	// @todo: weather particles
	// - list of texture IDs
	// - list of vertex buffer ids (all in 2D and in model space)
	// - list of transforms for screen positions

	std::vector<RenderDrawCall> drawCalls;

	ObjectTextureID getVoxelTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getEntityTextureID(const TextureAsset &textureAsset, bool flipped, bool reflective) const;
	ObjectTextureID getChasmFloorTextureID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(ArenaTypes::ChasmType chasmType, const TextureAsset &textureAsset) const;

	void loadTextures(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, TextureManager &textureManager,
		Renderer &renderer);
	void loadVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double chasmAnimPercent,
		bool nightLightsAreActive, RendererSystem3D &renderer);
	/*void loadEntities(const LevelInstance &levelInst, const RenderCamera &camera,
		const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive, bool playerHasLight,
		RendererSystem3D &renderer);
	void loadSky(const SkyInstance &skyInst, double daytimePercent, double latitude,
		RendererSystem3D &renderer);
	void loadWeather(const SkyInstance &skyInst, double daytimePercent, RendererSystem3D &renderer);

	// Updates dirty voxels (open doors, fading voxels, chasm animations, etc.), adds new ones,
	// and removes deleted ones.
	void updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double chasmAnimPercent,
		bool nightLightsAreActive, RendererSystem3D &renderer);

	// Updates entities that changed between frames, adds new entities, and removes deleted ones.
	// @todo: EntityManager should keep public lists of newEntityIDs and deletedEntityIDs and clear them each frame.
	void updateEntities(const LevelInstance &levelInst, const RenderCamera &camera,
		const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive,
		bool playerHasLight, RendererSystem3D &renderer);

	// Updates sky rotation, animations, and thunderstorm resources.
	void updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude);

	// Updates weather particles.
	void updateWeather(const SkyInstance &skyInst);*/
public:
	// Gets the list of draw calls for visible geometry this frame.
	BufferView<const RenderDrawCall> getDrawCalls() const;

	// Loads all the rendering resources of the given scene into the scene graph.
	void loadScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
		double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
		double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
		TextureManager &textureManager, Renderer &renderer, RendererSystem3D &renderer3D);

	// Clears all rendering resources from the scene graph (voxels, entities, sky, weather).
	void unloadScene(RendererSystem3D &renderer);

	// Updates rendering resources for anything in the scene that changed between frames.
	// I.e. dirty voxels, entities, sky rotation and animations, and weather particles.
	// @todo: get loadScene()/unloadScene() pattern working well before attempting to work with dirty voxels, etc.
	/*void updateScene(const LevelInstance &levelInst, const SkyInstance &skyInst, 
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
		double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
		double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
		TextureManager &textureManager, RendererSystem3D &renderer);*/
};

#endif
