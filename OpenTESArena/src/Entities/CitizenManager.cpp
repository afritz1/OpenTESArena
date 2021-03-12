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
	/*auto &gameState = game.getGameState();
	auto &worldData = gameState.getActiveWorld();
	const MapType activeMapType = worldData.getActiveMapType();
	return (activeMapType == MapType::City) || (activeMapType == MapType::Wilderness);*/
}

void CitizenManager::spawnCitizens(int raceID, const ChunkInt2 &playerChunk, const ChunkManager &chunkManager,
	EntityManager &entityManager, const LocationDefinition &locationDef,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary, Random &random,
	TextureManager &textureManager, Renderer &renderer)
{
	const ClimateType climateType = [&locationDef]()
	{
		DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
		const auto &cityDef = locationDef.getCityDefinition();
		return cityDef.climateType;
	}();

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
	
	// Only two citizen entity definitions for a given climate, based on the gender.
	const EntityDefinition &maleEntityDef = entityDefLibrary.getDefinition(maleEntityDefID);
	const EntityDefinition &femaleEntityDef = entityDefLibrary.getDefinition(femaleEntityDefID);
	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	maleAnimInst.init(maleEntityDef.getAnimDef());
	femaleAnimInst.init(femaleEntityDef.getAnimDef());

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

	// Chunk coordinates around the player that citizens can spawn in.
	ChunkInt2 minSpawnChunk, maxSpawnChunk;
	constexpr int spawnChunkDistance = 1; // Don't spawn citizens farther away than one chunk.
	ChunkUtils::getSurroundingChunks(playerChunk, spawnChunkDistance, &minSpawnChunk, &maxSpawnChunk);

	constexpr int citizenCount = 150; // Arbitrary. @todo: maybe change this to citizens per chunk?
	for (int i = 0; i < citizenCount; i++)
	{
		// Find suitable spawn position; might not succeed if there is no available spot.
		const std::optional<CoordInt2> spawnCoord = [&chunkManager, &random, &minSpawnChunk, &maxSpawnChunk]()
			-> std::optional<CoordInt2>
		{
			constexpr int spawnTriesCount = 50;
			for (int spawnTry = 0; spawnTry < spawnTriesCount; spawnTry++)
			{
				const ChunkInt2 spawnChunk(
					minSpawnChunk.x + random.next((maxSpawnChunk.x - minSpawnChunk.x) + 1),
					minSpawnChunk.y + random.next((maxSpawnChunk.y - minSpawnChunk.y) + 1));
				const VoxelInt2 spawnVoxel(
					random.next(Chunk::WIDTH),
					random.next(Chunk::DEPTH));

				const Chunk *chunk = chunkManager.tryGetChunk(spawnChunk);
				DebugAssert(chunk != nullptr);

				const Chunk::VoxelID voxelID = chunk->getVoxel(spawnVoxel.x, 1, spawnVoxel.y);
				const Chunk::VoxelID groundVoxelID = chunk->getVoxel(spawnVoxel.x, 0, spawnVoxel.y);
				const VoxelDefinition &voxelDef = chunk->getVoxelDef(voxelID);
				const VoxelDefinition &groundVoxelDef = chunk->getVoxelDef(groundVoxelID);

				if ((voxelDef.type == ArenaTypes::VoxelType::None) &&
					(groundVoxelDef.type == ArenaTypes::VoxelType::Floor))
				{
					return CoordInt2(spawnChunk, spawnVoxel);
				}
			}

			// No spawn position found.
			return std::nullopt;
		}();

		if (!spawnCoord.has_value())
		{
			DebugLogWarning("Couldn't find spawn position for citizen " + std::to_string(i) + ".");
			continue;
		}

		const bool male = random.next(2) == 0;
		const EntityDefID entityDefID = male ? maleEntityDefID : femaleEntityDefID;
		const EntityDefinition &entityDef = male ? maleEntityDef : femaleEntityDef;
		const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();

		const uint16_t colorSeed = static_cast<uint16_t>(random.next());
		const Palette generatedPalette = ArenaAnimUtils::transformCitizenColors(
			raceID, colorSeed, basePalette, binaryAssetLibrary.getExeData());

		EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
		DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
		dynamicEntity->initCitizen(entityDefID, male ? maleAnimInst : femaleAnimInst, CardinalDirectionName::North);

		// Idle animation by default.
		const std::optional<int> defaultStateIndex =
			entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
		if (!defaultStateIndex.has_value())
		{
			DebugLogWarning("Couldn't get idle state index for citizen \"" + std::to_string(i) + "\".");
			continue;
		}

		EntityAnimationInstance &animInst = dynamicEntity->getAnimInstance();
		animInst.setStateIndex(*defaultStateIndex);

		auto citizenParams = std::make_unique<EntityAnimationInstance::CitizenParams>();
		citizenParams->palette = generatedPalette;
		animInst.setCitizenParams(std::move(citizenParams));

		// Note: since the entity pointer is being used directly, update the position last
		// in scope to avoid a dangling pointer problem in case it changes chunks (from 0, 0).
		const CoordDouble2 spawnCoordReal(spawnCoord->chunk, VoxelUtils::getVoxelCenter(spawnCoord->voxel));
		dynamicEntity->setPosition(spawnCoordReal, entityManager);
	}

	// Initializes base male and female textures in the renderer.
	auto writeTextures = [&entityDefLibrary, &textureManager, &renderer, &maleEntityDef, &femaleEntityDef](bool male)
	{
		const EntityDefinition &entityDef = male ? maleEntityDef : femaleEntityDef;
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		for (int i = 0; i < animDef.getStateCount(); i++)
		{
			const EntityAnimationDefinition::State &state = animDef.getState(i);
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
				const bool flipped = keyframeList.isFlipped();
				for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
				{
					const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
					const TextureAssetReference &textureAssetRef = keyframe.getTextureAssetRef();
					constexpr bool reflective = false; // Citizens are not puddles.
					// @todo: duplicate texture check in some CitizenManager::rendererEntityTextureCache
					if (!renderer.tryCreateEntityTexture(textureAssetRef, flipped, reflective, textureManager))
					{
						DebugLogError("Couldn't create renderer entity texture for \"" + textureAssetRef.filename + "\".");
					}
				}
			}
		}
	};

	writeTextures(true);
	writeTextures(false);
}

void CitizenManager::clearCitizens(EntityManager &entityManager)
{
	Buffer<const Entity*> entities(entityManager.getCountOfType(EntityType::Dynamic));
	const int entityWriteCount = entityManager.getEntitiesOfType(
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
			auto &gameState = game.getGameState();
			const auto &player = gameState.getPlayer();
			auto &mapInst = gameState.getActiveMapInst();
			auto &levelInst = mapInst.getActiveLevel();
			const auto &chunkManager = levelInst.getChunkManager();
			auto &entityManager = levelInst.getEntityManager();
			const auto &provinceDef = gameState.getProvinceDefinition();
			const auto &locationDef = gameState.getLocationDefinition();
			const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			auto &random = game.getRandom();
			auto &textureManager = game.getTextureManager();
			auto &renderer = game.getRenderer();
			this->spawnCitizens(provinceDef.getRaceID(), player.getPosition().chunk, chunkManager, entityManager,
				locationDef, entityDefLibrary, binaryAssetLibrary, random, textureManager, renderer);

			this->stateType = StateType::HasSpawned;
		}
	}
}
