#include <algorithm>

#include "CitizenManager.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTypes.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

CitizenManager::CitizenManager()
{
	this->stateType = StateType::WaitingToSpawn;
}

bool CitizenManager::shouldSpawn(Game &game) const
{
	if (this->stateType == StateType::HasSpawned)
	{
		return false;
	}

	// Only worry about tick-related spawning; spawning at level start is handled by level loading.
	return false;
	/*auto &gameData = game.getGameData();
	auto &worldData = gameData.getActiveWorld();
	const MapType activeMapType = worldData.getActiveMapType();
	return (activeMapType == MapType::City) || (activeMapType == MapType::Wilderness);*/
}

void CitizenManager::spawnCitizens(int raceID, const VoxelGrid &voxelGrid, EntityManager &entityManager,
	const LocationDefinition &locationDef, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
	Renderer &renderer)
{
	const ClimateType climateType = [&locationDef]()
	{
		DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
		const auto &cityDef = locationDef.getCityDefinition();
		return cityDef.climateType;
	}();

	auto tryMakeEntityAnimInst = [&binaryAssetLibrary, &textureManager, climateType](
		bool male, EntityAnimationInstance *outAnimInst)
	{
		EntityAnimationDefinition animDef;
		if (!ArenaAnimUtils::tryMakeCitizenAnims(climateType, male, binaryAssetLibrary.getExeData(),
			textureManager, &animDef, outAnimInst))
		{
			DebugLogWarning(std::string("Couldn't make citizen anims (male: ") + (male ? "yes" : "no") +
				", climate: " + std::to_string(static_cast<int>(climateType)) + ").");
			return false;
		}

		return true;
	};

	// Only two citizen entity definitions for a given climate, based on the gender.
	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	if (!tryMakeEntityAnimInst(true, &maleAnimInst))
	{
		DebugLogWarning("Couldn't make male citizen entity anim instance.");
		return;
	}

	if (!tryMakeEntityAnimInst(false, &femaleAnimInst))
	{
		DebugLogWarning("Couldn't make female citizen entity anim instance.");
		return;
	}

	// Citizen entity definitions are level-independent and stored in a library beforehand.
	static_assert(EntityDefinitionLibrary::supportsDefType(EntityDefinition::Type::Citizen));
	EntityDefinitionLibrary::Key maleEntityDefKey, femaleEntityDefKey;
	maleEntityDefKey.initCitizen(true, climateType);
	femaleEntityDefKey.initCitizen(false, climateType);

	EntityDefID maleEntityDefID, femaleEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(maleEntityDefKey, &maleEntityDefID) ||
		!entityDefLibrary.tryGetDefinitionID(femaleEntityDefKey, &femaleEntityDefID))
	{
		DebugLogWarning("Couldn't get citizen entity def ID from library.");
		return;
	}

	// Male and female citizen resource handles for animation frames.
	const EntityRenderID maleEntityRenderID = renderer.makeEntityRenderID();
	const EntityRenderID femaleEntityRenderID = renderer.makeEntityRenderID();

	// Base palette for citizens to generate from.
	const Palette &basePalette = [&textureManager]() -> const Palette&
	{
		const std::string &paletteName = ArenaPaletteName::Default;
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get default palette \"" + paletteName + "\".");
		}

		return textureManager.getPaletteHandle(*paletteID);
	}();

	constexpr int citizenCount = 150; // Arbitrary.
	for (int i = 0; i < citizenCount; i++)
	{
		// Find suitable spawn position; might not succeed if there is no available spot.
		bool foundSpawnPosition = false;
		const NewInt2 spawnPositionXZ = [&voxelGrid, &random, &foundSpawnPosition]()
		{
			constexpr int spawnTriesCount = 50;
			for (int spawnTry = 0; spawnTry < spawnTriesCount; spawnTry++)
			{
				const NewInt2 voxel(
					random.next() % voxelGrid.getWidth(),
					random.next() % voxelGrid.getDepth());

				const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, 1, voxel.y);
				const uint16_t groundVoxelID = voxelGrid.getVoxel(voxel.x, 0, voxel.y);

				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				const VoxelDefinition &groundVoxelDef = voxelGrid.getVoxelDef(groundVoxelID);

				if ((voxelDef.type == ArenaTypes::VoxelType::None) &&
					(groundVoxelDef.type == ArenaTypes::VoxelType::Floor))
				{
					foundSpawnPosition = true;
					return voxel;
				}
			}

			return NewInt2();
		}();

		if (!foundSpawnPosition)
		{
			DebugLogWarning("Couldn't find spawn position for citizen " + std::to_string(i) + ".");
			continue;
		}

		const bool male = (random.next() % 2) == 0;
		const EntityDefID entityDefID = male ? maleEntityDefID : femaleEntityDefID;
		const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
		const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();

		const uint16_t colorSeed = static_cast<uint16_t>(random.next());
		const Palette generatedPalette = ArenaAnimUtils::transformCitizenColors(
			raceID, colorSeed, basePalette, binaryAssetLibrary.getExeData());

		EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
		DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
		dynamicEntity->initCitizen(entityDefID, male ? maleAnimInst : femaleAnimInst, CardinalDirectionName::North);
		dynamicEntity->setRenderID(male ? maleEntityRenderID : femaleEntityRenderID);

		// Idle animation by default.
		int defaultStateIndex;
		if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &defaultStateIndex))
		{
			DebugLogWarning("Couldn't get idle state index for citizen \"" + std::to_string(i) + "\".");
			continue;
		}

		EntityAnimationInstance &animInst = dynamicEntity->getAnimInstance();
		animInst.setStateIndex(defaultStateIndex);

		auto citizenParams = std::make_unique<EntityAnimationInstance::CitizenParams>();
		citizenParams->palette = generatedPalette;
		animInst.setCitizenParams(std::move(citizenParams));

		// Note: since the entity pointer is being used directly, update the position last
		// in scope to avoid a dangling pointer problem in case it changes chunks (from 0, 0).
		const NewDouble2 positionXZ = VoxelUtils::getVoxelCenter(spawnPositionXZ);
		const CoordDouble2 coord = VoxelUtils::newPointToCoord(positionXZ);
		dynamicEntity->setPosition(coord, entityManager, voxelGrid);
	}

	// Initializes base male and female textures in the renderer.
	auto writeTextures = [&entityDefLibrary, &textureManager, &renderer, maleEntityDefID, femaleEntityDefID,
		maleEntityRenderID, femaleEntityRenderID, &maleAnimInst, &femaleAnimInst](bool male)
	{
		const EntityRenderID entityRenderID = male ? maleEntityRenderID : femaleEntityRenderID;
		const EntityDefID entityDefID = male ? maleEntityDefID : femaleEntityDefID;
		const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const EntityAnimationInstance &animInst = male ? maleAnimInst : femaleAnimInst;
		constexpr bool isPuddle = false;

		renderer.setFlatTextures(entityRenderID, animDef, animInst, isPuddle, textureManager);
	};

	writeTextures(true);
	writeTextures(false);
}

void CitizenManager::clearCitizens(EntityManager &entityManager)
{
	Buffer<const Entity*> entities(entityManager.getCount(EntityType::Dynamic));
	const int entityWriteCount = entityManager.getEntities(
		EntityType::Dynamic, entities.get(), entities.getCount());

	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		DebugAssert(entity->getEntityType() == EntityType::Dynamic);

		const DynamicEntity *dynamicEntity = static_cast<const DynamicEntity*>(entity);
		if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
		{
			entityManager.remove(dynamicEntity->getID());
		}
	}
}

void CitizenManager::tick(Game &game)
{
	// @todo: in the event some citizens are removed when a chunk is removed,
	// the citizen manager should re-populate the number of citizens that
	// were removed.

	// @todo: expand this very primitive first attempt.
	if (this->stateType == StateType::WaitingToSpawn)
	{
		if (this->shouldSpawn(game))
		{
			auto &gameData = game.getGameData();
			auto &worldData = gameData.getActiveWorld();
			auto &levelData = worldData.getActiveLevel();
			const auto &voxelGrid = levelData.getVoxelGrid();
			auto &entityManager = levelData.getEntityManager();
			const auto &provinceDef = gameData.getProvinceDefinition();
			const auto &locationDef = gameData.getLocationDefinition();
			const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			auto &random = game.getRandom();
			auto &textureManager = game.getTextureManager();
			auto &renderer = game.getRenderer();
			this->spawnCitizens(provinceDef.getRaceID(), voxelGrid, entityManager, locationDef, entityDefLibrary,
				binaryAssetLibrary, random, textureManager, renderer);

			this->stateType = StateType::HasSpawned;
		}
	}
}
