#ifndef CITIZEN_MANAGER_H
#define CITIZEN_MANAGER_H

// @todo: things to throw in here:
// - spawning N townspeople entities with some conditions (during the day, no enemies nearby).
// - iteration over entities of a certain type for turning them on/off due to a crime

// @todo: not sure yet if this should be in the level or GameData or Game.

// @todo: run townspeople texture generation algorithm in here; move the actual algorithm
// to some utils namespace if it isn't already.

class Game;
class LevelData;
class LocationDefinition;
class MiscAssets;
class Random;
class Renderer;
class TextureManager;

class CitizenManager
{
private:
	enum class StateType
	{
		WaitingToSpawn,
		HasSpawned
	};

	StateType stateType;
	// @todo: need to track changes in active world type (i.e. city -> wilderness).

	bool shouldSpawn(Game &game) const;
public:
	CitizenManager();

	void spawnCitizens(LevelData &levelData, const LocationDefinition &locationDef,
		const MiscAssets &miscAssets, Random &random, TextureManager &textureManager,
		Renderer &renderer);
	void clearCitizens(Game &game);
	void tick(Game &game);
};

#endif
