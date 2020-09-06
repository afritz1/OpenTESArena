#include <algorithm>

#include "CitizenManager.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../World/VoxelDataType.h"
#include "../World/WorldType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

CitizenManager::GenerationEntry::GenerationEntry(bool male, const Palette &palette,
	EntityRenderID entityRenderID)
{
	this->male = male;
	this->palette = palette;
	this->entityRenderID = entityRenderID;
}

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
	auto &worldData = gameData.getWorldData();
	const WorldType activeWorldType = worldData.getActiveWorldType();
	return (activeWorldType == WorldType::City) || (activeWorldType == WorldType::Wilderness);*/
}

const CitizenManager::GenerationEntry *CitizenManager::findGenerationEntry(bool male,
	const Palette &palette) const
{
	const auto iter = std::find_if(this->generationEntries.begin(), this->generationEntries.end(),
		[male, &palette](const GenerationEntry &entry)
	{
		return (entry.male == male) && (entry.palette == palette);
	});

	return (iter != this->generationEntries.end()) ? &(*iter) : nullptr;
}

void CitizenManager::spawnCitizens(LevelData &levelData, int raceID,
	const LocationDefinition &locationDef, const MiscAssets &miscAssets, Random &random,
	TextureManager &textureManager, TextureInstanceManager &textureInstManager, Renderer &renderer)
{
	// Clear any previously-generated citizen tuples.
	this->generationEntries.clear();

	auto &entityManager = levelData.getEntityManager();
	const auto &voxelGrid = levelData.getVoxelGrid();

	const ClimateType climateType = [&locationDef]()
	{
		DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
		const auto &cityDef = locationDef.getCityDefinition();
		return cityDef.climateType;
	}();

	auto tryMakeEntityData = [&miscAssets, &textureManager, &levelData, climateType](bool male,
		EntityDefinition *outDef, EntityAnimationInstance *outAnimInst)
	{
		EntityAnimationDefinition animDef;
		if (!ArenaAnimUtils::tryMakeCitizenAnims(male, climateType, levelData.getInfFile(),
			miscAssets, textureManager, &animDef, outAnimInst))
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

	// Base palette for citizens to generate from.
	const Palette &basePalette = [&textureManager]() -> const Palette&
	{
		const std::string &paletteName = PaletteFile::fromName(PaletteName::Default);
		PaletteID paletteID;
		if (!textureManager.tryGetPaletteID(paletteName.c_str(), &paletteID))
		{
			DebugCrash("Couldn't get default palette \"" + paletteName + "\".");
		}

		return textureManager.getPaletteHandle(paletteID);
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

				if ((voxelDef.dataType == VoxelDataType::None) &&
					(groundVoxelDef.dataType == VoxelDataType::Floor))
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

		const uint16_t colorSeed = static_cast<uint16_t>(random.next());
		const Palette generatedPalette = ArenaAnimUtils::transformCitizenColors(
			raceID, colorSeed, basePalette, miscAssets.getExeData());
		
		// See if this combination has already been generated.
		const GenerationEntry *generationEntryPtr = this->findGenerationEntry(male, generatedPalette);
		if (generationEntryPtr == nullptr)
		{
			// Allocate new renderer ID since this is a unique-looking citizen.
			const EntityRenderID newEntityRenderID = renderer.makeEntityRenderID();
			this->generationEntries.push_back(GenerationEntry(male, generatedPalette, newEntityRenderID));
			generationEntryPtr = &this->generationEntries.back();
		}

		EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
		DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
		dynamicEntity->initCitizen(entityDefID, male ? maleAnimInst : femaleAnimInst,
			CardinalDirectionName::North);
		dynamicEntity->setRenderID(generationEntryPtr->entityRenderID);

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

	// Initializes textures in the renderer for this citizen variation.
	auto writeTextures = [&textureManager, &textureInstManager, &renderer, &entityManager, maleEntityDefID,
		femaleEntityDefID, &maleAnimInst, &femaleAnimInst](const GenerationEntry &generationEntry)
	{
		const bool male = generationEntry.male;
		const Palette &palette = generationEntry.palette;
		const EntityRenderID entityRenderID = generationEntry.entityRenderID;
		const EntityDefID entityDefID = male ? maleEntityDefID : femaleEntityDefID;
		const EntityDefinition &entityDef = entityManager.getEntityDef(entityDefID);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const EntityAnimationInstance &animInst = male ? maleAnimInst : femaleAnimInst;
		const bool isPuddle = false;

		renderer.setFlatTextures(entityRenderID, animDef, animInst, isPuddle, palette,
			textureManager, textureInstManager);
	};

	for (const GenerationEntry &generationEntry : this->generationEntries)
	{
		writeTextures(generationEntry);
	}
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
	// @todo: in the event some citizens are removed when a chunk is removed,
	// the citizen manager should re-populate the number of citizens that
	// were removed.

	// @todo: expand this very primitive first attempt.
	if (this->stateType == StateType::WaitingToSpawn)
	{
		if (this->shouldSpawn(game))
		{
			auto &gameData = game.getGameData();
			auto &worldData = gameData.getWorldData();
			auto &levelData = worldData.getActiveLevel();
			const auto &provinceDef = gameData.getProvinceDefinition();
			const auto &locationDef = gameData.getLocationDefinition();
			const auto &miscAssets = game.getMiscAssets();
			auto &random = game.getRandom();
			auto &textureManager = game.getTextureManager();
			auto &textureInstManager = game.getTextureInstanceManager();
			auto &renderer = game.getRenderer();
			this->spawnCitizens(levelData, provinceDef.getRaceID(), locationDef, miscAssets,
				random, textureManager, textureInstManager, renderer);

			this->stateType = StateType::HasSpawned;
		}
	}
}
