#ifndef CITIZEN_UTILS_H
#define CITIZEN_UTILS_H

#include "EntityAnimationInstance.h"
#include "EntityUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Media/TextureUtils.h"
#include "../World/Coord.h"

class BinaryAssetLibrary;
class Chunk;
class EntityDefinition;
class EntityDefinitionLibrary;
class EntityManager;
class Random;

enum class CardinalDirectionName;

namespace CitizenUtils
{
	// Arbitrary values.
	constexpr int CITIZENS_PER_CHUNK = 30;
	constexpr int MAX_ACTIVE_CITIZENS = CITIZENS_PER_CHUNK * 9;

	// How far away a citizen will consider idling around the player.
	constexpr double IDLE_DISTANCE = 1.25;

	// Walking speed of citizens.
	constexpr double SPEED = 2.25;

	struct CitizenGenInfo
	{
		EntityDefID maleEntityDefID;
		EntityDefID femaleEntityDefID;
		const EntityDefinition *maleEntityDef;
		const EntityDefinition *femaleEntityDef;
		EntityAnimationInstance maleAnimInst;
		EntityAnimationInstance femaleAnimInst;
		PaletteID paletteID;
		int raceID;

		void init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID, const EntityDefinition *maleEntityDef,
			const EntityDefinition *femaleEntityDef, EntityAnimationInstance &&maleAnimInst,
			EntityAnimationInstance &&femaleAnimInst, PaletteID paletteID, int raceID);
	};

	// Helper functions for determining a citizen's walking direction.
	bool tryGetCitizenDirectionFromCardinalDirection(CardinalDirectionName directionName, NewDouble2 *outDirection);
	CardinalDirectionName getCitizenDirectionNameByIndex(int index);
	NewDouble2 getCitizenDirectionByIndex(int index);
	int getRandomCitizenDirectionIndex(Random &random);

	// Gets the number of citizens active in the world.
	int getCitizenCount(const EntityManager &entityManager);
	int getCitizenCountInChunk(const ChunkInt2 &chunk, const EntityManager &entityManager);

	CitizenGenInfo makeCitizenGenInfo(int raceID, ArenaTypes::ClimateType climateType,
		const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager);

	bool trySpawnCitizenInChunk(const Chunk &chunk, const CitizenGenInfo &citizenGenInfo, Random &random,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityManager &entityManager);

	// Used when the player commits a crime and the guards are called.
	void clearCitizens(EntityManager &entityManager);
}

#endif
