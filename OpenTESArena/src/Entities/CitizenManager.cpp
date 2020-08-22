#include "CitizenManager.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../World/VoxelDataType.h"
#include "../World/WorldType.h"

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

	auto &gameData = game.getGameData();
	auto &worldData = gameData.getWorldData();
	const WorldType activeWorldType = worldData.getActiveWorldType();
	return (activeWorldType == WorldType::City) || (activeWorldType == WorldType::Wilderness);
}

void CitizenManager::spawnCitizens(Game &game)
{
	auto &gameData = game.getGameData();
	auto &textureManager = game.getTextureManager();
	auto &worldData = gameData.getWorldData();
	auto &levelData = worldData.getActiveLevel();
	auto &voxelGrid = levelData.getVoxelGrid();
	auto &entityManager = levelData.getEntityManager();

	const ClimateType climateType = [&gameData]()
	{
		const auto &locationDef = gameData.getLocationDefinition();
		DebugAssert(locationDef.getType() == LocationDefinition::Type::City);

		const auto &cityDef = locationDef.getCityDefinition();
		return cityDef.climateType;
	}();

	auto tryMakeEntityData = [&game, &levelData, climateType](bool male, EntityDefinition *outDef,
		EntityAnimationInstance *outAnimInst)
	{
		EntityAnimationDefinition animDef;
		if (!ArenaAnimUtils::tryMakeCitizenAnims(male, climateType, levelData.getInfFile(),
			game.getMiscAssets(), game.getTextureManager(), &animDef, outAnimInst))
		{
			DebugLogWarning(std::string("Couldn't make citizen anims (male: ") + (male ? "yes" : "no") +
				", climate: " + std::to_string(static_cast<int>(climateType)) + ").");
			return false;
		}

		outDef->initCitizen(male, climateType, std::move(animDef));
		return true;
	};

	// Only two citizen entity definitions for a given climate, based on the gender.
	EntityDefinition maleEntityDef, femaleEntityDef;
	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	if (!tryMakeEntityData(true, &maleEntityDef, &maleAnimInst))
	{
		DebugLogWarning("Couldn't make male citizen entity data.");
		return;
	}

	if (!tryMakeEntityData(false, &femaleEntityDef, &femaleAnimInst))
	{
		DebugLogWarning("Couldn't make female citizen entity data.");
		return;
	}

	const EntityDefID maleEntityDefID = entityManager.addEntityDef(std::move(maleEntityDef));
	const EntityDefID femaleEntityDefID = entityManager.addEntityDef(std::move(femaleEntityDef));

	constexpr int citizenCount = 200;
	for (int i = 0; i < citizenCount; i++)
	{
		Random &random = game.getRandom();

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
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				if (voxelDef.dataType == VoxelDataType::None)
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
		const EntityDefinition &entityDef = entityManager.getEntityDef(entityDefID);
		const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();

		EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
		entityRef.get()->init(entityDefID, male ? maleAnimInst : femaleAnimInst);

		DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
		dynamicEntity->setDerivedType(DynamicEntityType::Citizen);
		dynamicEntity->setDirection(CardinalDirection::North);

		// @todo: run random NPC texture generation
		// - need: 1) climate, 2) gender, 3) check if variation already generated.
		// 'Variation' roughly means something like a hair/skin/shirt/pants tuple.

		// @todo: don't get TextureManager involved beyond the EntityAnimationDefinition image
		// IDs; all the generated textures should be managed either in here or in Renderer.
		// The CitizenManager will handle INSTANCE DATA (potentially shared, including texture
		// variations, excluding things like personality state) of citizens that doesn't otherwise
		// fit in the entity manager.

		// Idle animation by default.
		int defaultStateIndex;
		if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &defaultStateIndex))
		{
			DebugLogWarning("Couldn't get idle state index for citizen \"" + std::to_string(i) + "\".");
			continue;
		}

		const NewDouble2 positionXZ(
			static_cast<SNDouble>(spawnPositionXZ.x) + 0.50,
			static_cast<WEDouble>(spawnPositionXZ.y) + 0.50);
		entityRef.get()->setPosition(positionXZ, entityManager, voxelGrid);

		EntityAnimationInstance &animInst = entityRef.get()->getAnimInstance();
		animInst.setStateIndex(defaultStateIndex);
	}

	// Palette for renderer textures.
	const Palette &palette = [&textureManager]() -> const Palette&
	{
		const std::string &paletteName = PaletteFile::fromName(PaletteName::Default);
		PaletteID paletteID;
		if (!textureManager.tryGetPaletteID(paletteName.c_str(), &paletteID))
		{
			DebugCrash("Couldn't get default palette \"" + paletteName + "\".");
		}

		return textureManager.getPaletteHandle(paletteID);
	}();

	// Initialize renderer buffers for the entity animation then populate
	// all textures of the animation.
	auto &renderer = game.getRenderer();
	auto writeTextures = [&textureManager, &entityManager, maleEntityDefID, femaleEntityDefID,
		&maleAnimInst, &femaleAnimInst, &palette, &renderer](bool male)
	{
		// @todo: remove renderer dependency on flatIndex and use some better unique identifier.
		// - maybe something the renderer considers unique? EntityRenderID? Influenced by entity instance.
		const int flatIndex = EntityDefinition::makeTempCitizenFlatIndex(male);
		const EntityDefID entityDefID = male ? maleEntityDefID : femaleEntityDefID;
		const EntityDefinition &entityDef = entityManager.getEntityDef(entityDefID);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const EntityAnimationInstance &animInst = male ? maleAnimInst : femaleAnimInst;

		// @todo: move this code into something reusable between this and LevelData::setActive().
		renderer.initFlatTextures(flatIndex, animInst);
		for (int stateIndex = 0; stateIndex < animInst.getStateCount(); stateIndex++)
		{
			const EntityAnimationDefinition::State &defState = animDef.getState(stateIndex);
			const EntityAnimationInstance::State &instState = animInst.getState(stateIndex);
			const int keyframeListCount = defState.getKeyframeListCount();

			for (int keyframeListIndex = 0; keyframeListIndex < keyframeListCount; keyframeListIndex++)
			{
				const EntityAnimationDefinition::KeyframeList &defKeyframeList =
					defState.getKeyframeList(keyframeListIndex);
				const EntityAnimationInstance::KeyframeList &keyframeList =
					instState.getKeyframeList(keyframeListIndex);
				const int keyframeCount = defKeyframeList.getKeyframeCount();
				const bool flipped = defKeyframeList.isFlipped();

				for (int keyframeIndex = 0; keyframeIndex < keyframeCount; keyframeIndex++)
				{
					const EntityAnimationInstance::Keyframe &keyframe =
						keyframeList.getKeyframe(keyframeIndex);
					const int stateID = stateIndex;
					const int angleID = keyframeListIndex;
					const int keyframeID = keyframeIndex;

					// Get texture associated with image ID and write texture data
					// to the renderer.
					const ImageID imageID = keyframe.getImageID();
					const Image &image = textureManager.getImageHandle(imageID);
					const bool isPuddle = false;
					renderer.setFlatTexture(flatIndex, stateID, angleID, keyframeID, flipped,
						image.getPixels(), image.getWidth(), image.getHeight(), isPuddle, palette);
				}
			}
		}
	};

	writeTextures(true);
	writeTextures(false);
}

void CitizenManager::clearCitizens(Game &game)
{
	auto &gameData = game.getGameData();
	auto &worldData = gameData.getWorldData();
	auto &levelData = worldData.getActiveLevel();
	auto &entityManager = levelData.getEntityManager();

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
	// @todo: expand this very primitive first attempt.
	if (this->stateType == StateType::WaitingToSpawn)
	{
		if (this->shouldSpawn(game))
		{
			this->spawnCitizens(game);
			this->stateType = StateType::HasSpawned;
		}
	}
}
