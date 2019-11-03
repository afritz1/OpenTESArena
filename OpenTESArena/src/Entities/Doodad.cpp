#include "Doodad.h"
#include "EntityType.h"
#include "../Game/Game.h"

Doodad::Doodad()
	: Entity(EntityType::Doodad) { }

void Doodad::tick(Game &game, double dt)
{
	// Get entity animation data.
	const EntityAnimationData &animationData = [this, &game]() -> const EntityAnimationData&
	{
		const auto &worldData = game.getGameData().getWorldData();
		const auto &levelData = worldData.getActiveLevel();
		const EntityManager &entityManager = levelData.getEntityManager();
		const EntityData *entityData = entityManager.getEntityData(this->getDataIndex());
		DebugAssert(entityData != nullptr);

		return entityData->getAnimationData();
	}();

	// Animate.
	auto &animation = this->getAnimation();
	animation.tick(dt, animationData);
}
