#ifndef MAP_INSTANCE_H
#define MAP_INSTANCE_H

#include "LevelInstance.h"
#include "../Entities/EntityGeneration.h"
#include "../Sky/SkyInstance.h"

#include "components/utilities/Buffer.h"

// Contains instance data for the associated map definition. This is the current state of voxels,
// entities, and sky for every level instance in the map.

class AudioManager;
class MapDefinition;
class TextureManager;

class MapInstance
{
private:
	Buffer<LevelInstance> levels;
	Buffer<SkyInstance> skies;
	int activeLevelIndex;
	int activeSkyIndex;

	void initInterior(const MapDefinition &mapDefinition, TextureManager &textureManager, Renderer &renderer);
	void initCity(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer);
	void initWild(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer);
public:
	MapInstance();

	void init(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer);

	int getLevelCount() const;
	LevelInstance &getLevel(int index);
	const LevelInstance &getLevel(int index) const;
	int getActiveLevelIndex() const; // For indexing into map definition.
	LevelInstance &getActiveLevel();
	const LevelInstance &getActiveLevel() const;
	
	int getSkyCount() const;
	SkyInstance &getSky(int index);
	const SkyInstance &getSky(int index) const;
	SkyInstance &getActiveSky();
	const SkyInstance &getActiveSky() const;

	void setActiveLevelIndex(int levelIndex, const MapDefinition &mapDefinition);

	void update(double dt, Game &game, const CoordDouble3 &playerCoord, const MapDefinition &mapDefinition,
		double latitude, double daytimePercent, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager);

	void cleanUp();
};

#endif
