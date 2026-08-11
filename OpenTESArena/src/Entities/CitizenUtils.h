#pragma once

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
	constexpr int MAX_CITIZENS_PER_CHUNK = 30; // Arbitrary.

	// The number of chunks times max citizens per chunk, plus a bit in case citizens have congregated in a few chunks.
	int getMaxCitizenCountForScene(int chunkCount);

	bool canMapTypeSpawnCitizens(MapType mapType);
	CitizenGenInfo makeCitizenGenInfo(int raceID, ArenaClimateType climateType);
	std::optional<CitizenGenInfo> tryMakeCitizenGenInfo(MapType mapType, int raceID, const LocationDefinition &locationDef);

	// Helper functions for determining a citizen's walking direction.
	CardinalDirectionName getCitizenDirectionNameByIndex(int index);
	Double3 getCitizenDirectionByIndex(int index);
	int getRandomCitizenDirectionIndex(Random &random);

	int getCitizenCountInChunk(ChunkInt2 chunkPos, const EntityChunkManager &entityChunkManager);
}
