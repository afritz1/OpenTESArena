#include <algorithm>

#include "CitizenUtils.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/VoxelChunk.h"
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
	const EntityDefinition *maleEntityDef, const EntityDefinition *femaleEntityDef,
	EntityAnimationInstance &&maleAnimInst, EntityAnimationInstance &&femaleAnimInst, PaletteID paletteID, int raceID)
{
	this->maleEntityDefID = maleEntityDefID;
	this->femaleEntityDefID = femaleEntityDefID;
	this->maleEntityDef = maleEntityDef;
	this->femaleEntityDef = femaleEntityDef;
	this->maleAnimInst = std::move(maleAnimInst);
	this->femaleAnimInst = std::move(femaleAnimInst);
	this->paletteID = paletteID;
	this->raceID = raceID;
}

bool CitizenUtils::canMapTypeSpawnCitizens(MapType mapType)
{
	return (mapType == MapType::City) || (mapType == MapType::Wilderness);
}

CitizenUtils::CitizenGenInfo CitizenUtils::makeCitizenGenInfo(int raceID, ArenaTypes::ClimateType climateType,
	const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager)
{
	// Citizen entity definitions are level-independent and stored in a library beforehand.
	static_assert(EntityDefinitionLibrary::supportsDefType(EntityDefinition::Type::Citizen));
	EntityDefinitionLibrary::Key maleEntityDefKey, femaleEntityDefKey;
	maleEntityDefKey.initCitizen(true, climateType);
	femaleEntityDefKey.initCitizen(false, climateType);

	EntityDefID maleEntityDefID, femaleEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(maleEntityDefKey, &maleEntityDefID) ||
		!entityDefLibrary.tryGetDefinitionID(femaleEntityDefKey, &femaleEntityDefID))
	{
		DebugCrash("Couldn't get citizen entity def ID from library.");
	}

	// Only two citizen entity definitions for a given climate, based on the gender.
	const EntityDefinition &maleEntityDef = entityDefLibrary.getDefinition(maleEntityDefID);
	const EntityDefinition &femaleEntityDef = entityDefLibrary.getDefinition(femaleEntityDefID);

	auto initAnimInst = [](EntityAnimationInstance &animInst, const EntityAnimationDefinition &animDef)
	{
		for (int i = 0; i < animDef.stateCount; i++)
		{
			const EntityAnimationDefinitionState &animDefState = animDef.states[i];
			animInst.addState(animDefState.seconds, animDefState.isLooping);
		}
		
		// Idle animation by default.
		const std::optional<int> stateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
		if (!stateIndex.has_value())
		{
			DebugLogError("Couldn't get idle state index for citizen.");
			return;
		}

		animInst.setStateIndex(*stateIndex);
	};

	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	initAnimInst(maleAnimInst, maleEntityDef.getAnimDef());
	initAnimInst(femaleAnimInst, femaleEntityDef.getAnimDef());	

	// Base palette for citizens to generate from.
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get default palette \"" + paletteName + "\".");
	}

	CitizenGenInfo citizenGenInfo;
	citizenGenInfo.init(maleEntityDefID, femaleEntityDefID, &maleEntityDef, &femaleEntityDef,
		std::move(maleAnimInst), std::move(femaleAnimInst), *paletteID, raceID);
	return citizenGenInfo;
}

std::optional<CitizenUtils::CitizenGenInfo> CitizenUtils::tryMakeCitizenGenInfo(MapType mapType, int raceID,
	const LocationDefinition &locationDef, const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager)
{
	if (!CitizenUtils::canMapTypeSpawnCitizens(mapType))
	{
		return std::nullopt;
	}

	const LocationDefinition::Type locationDefType = locationDef.getType();
	if (locationDefType != LocationDefinition::Type::City)
	{
		return std::nullopt;
	}
	
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const ArenaTypes::ClimateType climateType = cityDef.climateType;
	return CitizenUtils::makeCitizenGenInfo(raceID, climateType, entityDefLibrary, textureManager);
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

WorldDouble2 CitizenUtils::getCitizenDirectionByIndex(int index)
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
	return entityChunkManager.getCountInChunkWithPalette(chunkPos);
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
