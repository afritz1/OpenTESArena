#include <algorithm>

#include "ArenaAnimUtils.h"
#include "CitizenUtils.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/VoxelChunk.h"
#include "../World/CardinalDirection.h"
#include "../World/CardinalDirectionName.h"
#include "../World/MapType.h"
#include "../WorldMap/ProvinceDefinition.h"

#include "components/utilities/Buffer.h"

namespace
{
	// Allowed directions for citizens to walk.
	constexpr std::pair<CardinalDirectionName, Double3> CitizenDirections[] =
	{
		{ CardinalDirectionName::North, Double3(CardinalDirection::North.x, 0.0, CardinalDirection::North.y) },
		{ CardinalDirectionName::East, Double3(CardinalDirection::East.x, 0.0, CardinalDirection::East.y) },
		{ CardinalDirectionName::South, Double3(CardinalDirection::South.x, 0.0, CardinalDirection::South.y) },
		{ CardinalDirectionName::West, Double3(CardinalDirection::West.x, 0.0, CardinalDirection::West.y) }
	};

	static_assert(sizeof(CardinalDirection::North) == sizeof(double) * 2);
}

void CitizenGenInfo::init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID,
	const EntityDefinition *maleEntityDef, const EntityDefinition *femaleEntityDef, int raceID)
{
	this->maleEntityDefID = maleEntityDefID;
	this->femaleEntityDefID = femaleEntityDefID;
	this->maleEntityDef = maleEntityDef;
	this->femaleEntityDef = femaleEntityDef;
	this->raceID = raceID;
}

int CitizenUtils::getMaxCitizenCountForScene(int chunkCount)
{
	DebugAssert(chunkCount >= 0);
	const int baseMaxCitizensPerScene = CitizenUtils::MAX_CITIZENS_PER_CHUNK * chunkCount;
	return (5 * baseMaxCitizensPerScene) / 4;
}

bool CitizenUtils::canMapTypeSpawnCitizens(MapType mapType)
{
	return (mapType == MapType::City) || (mapType == MapType::Wilderness);
}

CitizenGenInfo CitizenUtils::makeCitizenGenInfo(int raceID, ArenaClimateType climateType)
{
	// Citizen entity definitions are level-independent and stored in a library beforehand.
	static_assert(EntityDefinitionLibrary::supportsDefType(EntityDefinitionType::Citizen));
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	EntityDefinitionKey maleEntityDefKey, femaleEntityDefKey;
	maleEntityDefKey.initCitizen(true, climateType);
	femaleEntityDefKey.initCitizen(false, climateType);

	EntityDefID maleEntityDefID, femaleEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(maleEntityDefKey, &maleEntityDefID) ||
		!entityDefLibrary.tryGetDefinitionID(femaleEntityDefKey, &femaleEntityDefID))
	{
		DebugCrash("Couldn't get citizen entity def ID from library.");
	}

	// Two citizen entity definitions per climate.
	const EntityDefinition &maleEntityDef = entityDefLibrary.getDefinition(maleEntityDefID);
	const EntityDefinition &femaleEntityDef = entityDefLibrary.getDefinition(femaleEntityDefID);

	CitizenGenInfo citizenGenInfo;
	citizenGenInfo.init(maleEntityDefID, femaleEntityDefID, &maleEntityDef, &femaleEntityDef, raceID);
	return citizenGenInfo;
}

std::optional<CitizenGenInfo> CitizenUtils::tryMakeCitizenGenInfo(MapType mapType, int raceID, const LocationDefinition &locationDef)
{
	if (!CitizenUtils::canMapTypeSpawnCitizens(mapType))
	{
		return std::nullopt;
	}

	const LocationDefinitionType locationDefType = locationDef.getType();
	if (locationDefType != LocationDefinitionType::City)
	{
		return std::nullopt;
	}
	
	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	const ArenaClimateType climateType = cityDef.climateType;
	return CitizenUtils::makeCitizenGenInfo(raceID, climateType);
}

CardinalDirectionName CitizenUtils::getCitizenDirectionNameByIndex(int index)
{
	DebugAssertIndex(CitizenDirections, index);
	return CitizenDirections[index].first;
}

Double3 CitizenUtils::getCitizenDirectionByIndex(int index)
{
	DebugAssertIndex(CitizenDirections, index);
	return CitizenDirections[index].second;
}

int CitizenUtils::getRandomCitizenDirectionIndex(Random &random)
{
	return random.next(static_cast<int>(std::size(CitizenDirections)));
}

int CitizenUtils::getCitizenCountInChunk(ChunkInt2 chunkPos, const EntityChunkManager &entityChunkManager)
{
	int count = 0;
	for (const EntityInstanceID entityInstID : entityChunkManager.citizenEntityInstIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
		const WorldDouble3 &entityPosition = entityChunkManager.positions.get(entityInst.positionID);
		const ChunkInt2 entityChunkPos = VoxelUtils::worldPointToChunk(entityPosition);
		if (entityChunkPos == chunkPos)
		{
			count++;
		}
	}

	return count;
}
