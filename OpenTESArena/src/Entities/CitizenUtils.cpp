#include <algorithm>

#include "CitizenUtils.h"
#include "EntityDefinitionLibrary.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"

namespace
{
	// Allowed directions for citizens to walk.
	const std::array<std::pair<CardinalDirectionName, NewDouble2>, 4> CitizenDirections =
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

bool CitizenUtils::tryGetCitizenDirectionFromCardinalDirection(CardinalDirectionName directionName, NewDouble2 *outDirection)
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

NewDouble2 CitizenUtils::getCitizenDirectionByIndex(int index)
{
	DebugAssertIndex(CitizenDirections, index);
	return CitizenDirections[index].second;
}

int CitizenUtils::getRandomCitizenDirectionIndex(Random &random)
{
	return random.next() % static_cast<int>(CitizenDirections.size());
}

int CitizenUtils::getCitizenCount(const EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountOfType(entityType));
	const int entityWriteCount = entityManager.getEntitiesOfType(entityType, entities.get(), entities.getCount());

	int count = 0;
	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
		if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
		{
			count++;
		}
	}

	return count;
}

int CitizenUtils::getCitizenCountInChunk(const ChunkInt2 &chunk, const EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountInChunk(chunk));
	const int entityWriteCount = entityManager.getEntitiesInChunk(chunk, entities.get(), entities.getCount());

	int count = 0;
	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		if (entity->getEntityType() == entityType)
		{
			const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
			if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
			{
				count++;
			}
		}
	}

	return count;
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
	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	maleAnimInst.init(maleEntityDef.getAnimDef());
	femaleAnimInst.init(femaleEntityDef.getAnimDef());

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

bool CitizenUtils::trySpawnCitizenInChunk(const Chunk &chunk, const CitizenGenInfo &citizenGenInfo, Random &random,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityManager &entityManager)
{
	const ChunkInt2 &chunkPosition = chunk.getPosition();
	if (!entityManager.hasChunk(chunkPosition))
	{
		DebugLogWarning("Can't spawn a citizen in untracked chunk \"" + chunkPosition.toString() + "\".");
		return false;
	}

	const std::optional<VoxelInt2> spawnVoxel = [&chunk, &random]() -> std::optional<VoxelInt2>
	{
		DebugAssert(chunk.getHeight() >= 2);

		constexpr int spawnTriesCount = 20;
		for (int spawnTry = 0; spawnTry < spawnTriesCount; spawnTry++)
		{
			const VoxelInt2 spawnVoxel(random.next(Chunk::WIDTH), random.next(Chunk::DEPTH));
			const Chunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getVoxelTraitsDefID(spawnVoxel.x, 1, spawnVoxel.y);
			const Chunk::VoxelTraitsDefID groundVoxelTraitsDefID = chunk.getVoxelTraitsDefID(spawnVoxel.x, 0, spawnVoxel.y);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk.getVoxelTraitsDef(voxelTraitsDefID);
			const VoxelTraitsDefinition &groundVoxelTraitsDef = chunk.getVoxelTraitsDef(groundVoxelTraitsDefID);

			// @todo: this type check could ostensibly be replaced with some "allowsCitizenSpawn".
			if ((voxelTraitsDef.type == ArenaTypes::VoxelType::None) &&
				(groundVoxelTraitsDef.type == ArenaTypes::VoxelType::Floor))
			{
				return spawnVoxel;
			}
		}

		// No spawn position found.
		return std::nullopt;
	}();

	if (!spawnVoxel.has_value())
	{
		DebugLogWarning("Couldn't find spawn voxel for citizen.");
		return false;
	}

	const bool male = random.next(2) == 0;
	const EntityDefID entityDefID = male ? citizenGenInfo.maleEntityDefID : citizenGenInfo.femaleEntityDefID;
	const EntityDefinition &entityDef = male ? *citizenGenInfo.maleEntityDef : *citizenGenInfo.femaleEntityDef;
	const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();
	const EntityAnimationInstance &entityAnimInst = male ? citizenGenInfo.maleAnimInst : citizenGenInfo.femaleAnimInst;

	const Palette &palette = textureManager.getPaletteHandle(citizenGenInfo.paletteID);
	const uint16_t colorSeed = static_cast<uint16_t>(random.next());
	const Palette generatedPalette = ArenaAnimUtils::transformCitizenColors(
		citizenGenInfo.raceID, colorSeed, palette, binaryAssetLibrary.getExeData());

	EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
	DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
	constexpr CardinalDirectionName direction = CardinalDirectionName::North;
	dynamicEntity->initCitizen(entityDefID, entityAnimInst, direction);

	// Idle animation by default.
	const std::optional<int> stateIndex = entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	if (!stateIndex.has_value())
	{
		DebugLogWarning("Couldn't get idle state index for citizen.");
		return false;
	}

	EntityAnimationInstance &animInst = dynamicEntity->getAnimInstance();
	animInst.setStateIndex(*stateIndex);

	auto citizenParams = std::make_unique<EntityAnimationInstance::CitizenParams>();
	citizenParams->palette = generatedPalette;
	animInst.setCitizenParams(std::move(citizenParams));

	// Note: since the entity pointer is being used directly, update the position last
	// in scope to avoid a dangling pointer problem in case it changes chunks.
	const CoordDouble2 spawnCoordReal(chunkPosition, VoxelUtils::getVoxelCenter(*spawnVoxel));
	dynamicEntity->setPosition(spawnCoordReal, entityManager);

	return true;
}

void CitizenUtils::clearCitizens(EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountOfType(entityType));
	const int entityWriteCount = entityManager.getEntitiesOfType(entityType, entities.get(), entities.getCount());

	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
		if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
		{
			entityManager.remove(dynamicEntity->getID());
		}
	}
}
