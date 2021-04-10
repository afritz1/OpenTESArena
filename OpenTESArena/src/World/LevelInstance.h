#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <optional>

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

enum class MapType;

class LevelInstance
{
private:
	ChunkManager chunkManager;
	EntityManager entityManager;
	double ceilingScale;
public:
	LevelInstance();

	void init(double ceilingScale);

	ChunkManager &getChunkManager();
	const ChunkManager &getChunkManager() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	double getCeilingScale() const;

	bool trySetActive(ArenaTypes::WeatherType weatherType, bool nightLightsAreActive,
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
