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
	const std::array<std::pair<CardinalDirectionName, WorldDouble2>, 4> CitizenDirections =
	{
		{
			{ CardinalDirectionName::North, CardinalDirection::North },
			{ CardinalDirectionName::East, CardinalDirection::East },
			{ CardinalDirectionName::South, CardinalDirection::South },
			{ CardinalDirectionName::West, CardinalDirection::West }
		}
	};
}

void CitizenUtils::CitizenGenInfo::init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID,
	const EntityDefinition *maleEntityDef, const EntityDefinition *femaleEntityDef, int raceID)
{
	this->maleEntityDefID = maleEntityDefID;
	this->femaleEntityDefID = femaleEntityDefID;
	this->maleEntityDef = maleEntityDef;
	this->femaleEntityDef = femaleEntityDef;
	this->raceID = raceID;
}

bool CitizenUtils::canMapTypeSpawnCitizens(MapType mapType)
{
	return (mapType == MapType::City) || (mapType == MapType::Wilderness);
}

CitizenUtils::CitizenGenInfo CitizenUtils::makeCitizenGenInfo(int raceID, ArenaTypes::ClimateType climateType)
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

std::optional<CitizenUtils::CitizenGenInfo> CitizenUtils::tryMakeCitizenGenInfo(MapType mapType, int raceID,
	const LocationDefinition &locationDef)
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
	const ArenaTypes::ClimateType climateType = cityDef.climateType;
	return CitizenUtils::makeCitizenGenInfo(raceID, climateType);
}

bool CitizenUtils::tryGetCitizenDirectionFromCardinalDirection(CardinalDirectionName directionName, WorldDouble2 *outDirection)
{
	const auto iter = std::find_if(CitizenDirections.begin(), CitizenDirections.end(),
		[directionName](const auto &pair)
	{
		return pair.first == directionName;
	});

	if (iter != CitizenDirections.end())
	{
		*outDirection = iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

CardinalDirectionName CitizenUtils::getCitizenDirectionNameByIndex(int index)
{
	DebugAssertIndex(CitizenDirections, index);
	return CitizenDirections[index].first;
}

const WorldDouble2 &CitizenUtils::getCitizenDirectionByIndex(int index)
{
	DebugAssertIndex(CitizenDirections, index);
	return CitizenDirections[index].second;
}

int CitizenUtils::getRandomCitizenDirectionIndex(Random &random)
{
	return random.next() % static_cast<int>(CitizenDirections.size());
}

int CitizenUtils::getCitizenCountInChunk(const ChunkInt2 &chunkPos, const EntityChunkManager &entityChunkManager)
{
	return entityChunkManager.getCountInChunkWithCitizenDirection(chunkPos);
}

int CitizenUtils::getCitizenCount(const EntityChunkManager &entityChunkManager)
{
	int count = 0;
	for (int i = 0; i < entityChunkManager.getChunkCount(); i++)
	{
		const EntityChunk &chunk = entityChunkManager.getChunkAtIndex(i);
		count += CitizenUtils::getCitizenCountInChunk(chunk.getPosition(), entityChunkManager);
	}

	return count;
}
