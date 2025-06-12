#ifndef CITIZEN_UTILS_H
#define CITIZEN_UTILS_H

#include <optional>

#include "EntityAnimationInstance.h"
#include "EntityUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureUtils.h"
#include "../World/Coord.h"

class EntityChunkManager;
class LocationDefinition;
class Random;

enum class CardinalDirectionName;
enum class MapType;

struct EntityDefinition;

struct CitizenGenInfo
{
	EntityDefID maleEntityDefID;
	EntityDefID femaleEntityDefID;
	const EntityDefinition *maleEntityDef;
	const EntityDefinition *femaleEntityDef;
	int raceID;

	void init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID, const EntityDefinition *maleEntityDef,
		const EntityDefinition *femaleEntityDef, int raceID);
};

namespace CitizenUtils
{
	// Arbitrary values.
	constexpr int CITIZENS_PER_CHUNK = 30;
	constexpr int MAX_ACTIVE_CITIZENS = CITIZENS_PER_CHUNK * 9;

	bool canMapTypeSpawnCitizens(MapType mapType);
	CitizenGenInfo makeCitizenGenInfo(int raceID, ArenaClimateType climateType);
	std::optional<CitizenGenInfo> tryMakeCitizenGenInfo(MapType mapType, int raceID, const LocationDefinition &locationDef);

	// Helper functions for determining a citizen's walking direction.
	bool tryGetCitizenDirectionFromCardinalDirection(CardinalDirectionName directionName, WorldDouble2 *outDirection);
	CardinalDirectionName getCitizenDirectionNameByIndex(int index);
	const WorldDouble2 &getCitizenDirectionByIndex(int index);
	int getRandomCitizenDirectionIndex(Random &random);

	// Gets the number of citizens active in the world or a chunk.
	int getCitizenCountInChunk(const ChunkInt2 &chunkPos, const EntityChunkManager &entityChunkManager);
	int getCitizenCount(const EntityChunkManager &entityChunkManager);
}

#endif
