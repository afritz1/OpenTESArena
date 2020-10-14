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
class Game;
class LevelData;
class LocationDefinition;
class Random;
class Renderer;
class TextureInstanceManager;
class TextureManager;

class CitizenManager
{
private:
	enum class StateType
	{
		WaitingToSpawn,
		HasSpawned
	};

	// Instance of unique-looking citizen info for the current world. Allows identical-looking
	// citizens to share the same render ID.
	struct GenerationEntry
	{
		bool male;
		Palette palette;
		EntityRenderID entityRenderID;

		GenerationEntry(bool male, const Palette &palette, EntityRenderID entityRenderID);
	};

	StateType stateType;
	std::vector<GenerationEntry> generationEntries;
	// @todo: need to track changes in active world type (i.e. city -> wilderness).

	bool shouldSpawn(Game &game) const;
	const GenerationEntry *findGenerationEntry(bool male, const Palette &palette) const;
public:
	CitizenManager();

	void spawnCitizens(LevelData &levelData, int raceID, const LocationDefinition &locationDef,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		Random &random, TextureManager &textureManager, TextureInstanceManager &textureInstManager,
		Renderer &renderer);
	void clearCitizens(Game &game);
	void tick(Game &game);
};

#endif
