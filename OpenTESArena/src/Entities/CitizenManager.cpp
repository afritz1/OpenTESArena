#include "CitizenManager.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"
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
	auto &worldData = gameData.getWorldData();
	auto &levelData = worldData.getActiveLevel();
	auto &entityManager = levelData.getEntityManager();

	// @todo: run random NPC texture generation
	// - need: 1) climate, 2) gender, 3) check if variation already generated.
	// 'Variation' roughly means something like hair/skin/shirt/pants.

	// @todo: don't get TextureManager involved beyond the EntityAnimationDefinition image
	// IDs; all the generated textures should be managed either in here or in Renderer.
	// The CitizenManager will handle INSTANCE DATA (potentially shared, including texture
	// variations, excluding things like personality state) of citizens that doesn't otherwise
	// fit in the entity manager.

	DebugLog("Spawned citizens.");
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
