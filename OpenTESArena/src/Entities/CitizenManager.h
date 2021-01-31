#ifndef CITIZEN_MANAGER_H
#define CITIZEN_MANAGER_H

#include <vector>

#include "EntityUtils.h"
#include "../Media/Palette.h"

// @todo: things to throw in here:
// - spawning N townspeople entities with some conditions (during the day, no enemies nearby).
// - iteration over entities of a certain type for turning them on/off due to a crime

// @todo: not sure yet if this should be in the level or GameData or Game.

class BinaryAssetLibrary;
class EntityAnimationDefinition;
class EntityAnimationInstance;
class EntityDefinitionLibrary;
class EntityManager;
class Game;
class LocationDefinition;
class Random;
class Renderer;
class TextureManager;
class VoxelGrid;

class CitizenManager
{
private:
	enum class StateType
	{
		WaitingToSpawn,
		HasSpawned
	};

	StateType stateType;
	// @todo: maybe need vector of EntityID to keep track of spawned citizens.
	// @todo: need to track changes in active world type (i.e. city -> wilderness).

	bool shouldSpawn(Game &game) const;
public:
	CitizenManager();

	void spawnCitizens(int raceID, const VoxelGrid &voxelGrid, EntityManager &entityManager,
		const LocationDefinition &locationDef, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
		Renderer &renderer);
	void clearCitizens(EntityManager &entityManager);
	void tick(Game &game);
};

#endif
