#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "ChunkManager.h"
#include "../Assets/ArenaTypes.h"
#include "../Entities/CitizenUtils.h"
#include "../Entities/EntityGeneration.h"
#include "../Entities/EntityManager.h"

// Instance of a level with voxels and entities. Its data is in a baked, context-sensitive format
// and depends on one or more level definitions for its population.

class AudioManager;
class EntityDefinitionLibrary;
class MapDefinition;
class Renderer;
class WeatherDefinition;

enum class MapType;

class LevelInstance
{
private:
	// @todo: problem to consider here:
	// - why do we load voxel and entity textures before they are instantiated in the world?
	// - we make the assumption that "a level has voxel and entity textures" but that is decoupled from actual voxel and entity instances.
	// - feels like all voxel/entity/sky/particle object texture loading should be on demand...? Might simplify enemy spawning code.

	struct LoadedVoxelMaterial
	{
		TextureAssetReference textureAssetRef;

		// One texture per material.
		ScopedObjectTextureRef objectTextureRef;
		ScopedObjectMaterialRef objectMaterialRef;

		void init(const TextureAssetReference &textureAssetRef, ScopedObjectTextureRef &&objectTextureRef,
			ScopedObjectMaterialRef &&objectMaterialRef);
	};

	struct LoadedEntityMaterial
	{
		TextureAssetReference textureAssetRef;
		bool flipped;
		bool reflective;

		// One texture per material.
		ScopedObjectTextureRef objectTextureRef;
		ScopedObjectMaterialRef objectMaterialRef;

		void init(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective,
			ScopedObjectTextureRef &&objectTextureRef, ScopedObjectMaterialRef &&objectMaterialRef);
	};

	struct LoadedChasmMaterialList
	{
		struct Entry
		{
			// Two textures per material.
			ScopedObjectTextureRef objectTextureRef0, objectTextureRef1;
			ScopedObjectMaterialRef objectMaterialRef;

			void init(ScopedObjectTextureRef &&objectTextureRef0, ScopedObjectTextureRef &&objectTextureRef1,
				ScopedObjectMaterialRef &&objectMaterialRef);
		};

		ArenaTypes::ChasmType chasmType;
		std::vector<Entry> entries; // One per animation frame.
		
		void init(ArenaTypes::ChasmType chasmType);
		void add(Entry &&entry);
	};

	ChunkManager chunkManager;
	EntityManager entityManager;

	// Renderer resources.
	std::vector<LoadedVoxelMaterial> voxelMaterials;
	std::vector<LoadedEntityMaterial> entityMaterials;
	std::vector<LoadedChasmMaterialList> chasmMaterialLists;

	// Texture handles for the active game world palette and light table.
	ScopedObjectTextureRef paletteTextureRef, lightTableTextureRef;

	double ceilingScale;
public:
	LevelInstance();

	void init(double ceilingScale);

	ChunkManager &getChunkManager();
	const ChunkManager &getChunkManager() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	ObjectTextureID getPaletteTextureID() const;
	ObjectTextureID getLightTableTextureID() const;
	double getCeilingScale() const;

	ObjectMaterialID getVoxelMaterialID(const TextureAssetReference &textureAssetRef) const;
	ObjectMaterialID getEntityMaterialID(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective) const;
	ObjectMaterialID getChasmMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const;

	bool trySetActive(const WeatherDefinition &weatherDef, bool nightLightsAreActive,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		TextureManager &textureManager, Renderer &renderer);

	void update(double dt, Game &game, const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, int chunkDistance,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, AudioManager &audioManager);
};

#endif
