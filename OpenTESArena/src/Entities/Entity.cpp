#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"

Entity::Entity()
	: position(Double2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->dataIndex = -1;
	this->animation.reset();
}

void Entity::init(int dataIndex)
{
	DebugAssert(this->id != EntityManager::NO_ID);
	this->dataIndex = dataIndex;
}

EntityID Entity::getID() const
{
	return this->id;
}

int Entity::getDataIndex() const
{
	return this->dataIndex;
}

const NewDouble2 &Entity::getPosition() const
{
	return this->position;
}

EntityAnimationData::Instance &Entity::getAnimation()
{
	return this->animation;
}

const EntityAnimationData::Instance &Entity::getAnimation() const
{
	return this->animation;
}

void Entity::setID(EntityID id)
{
	this->id = id;
}

void Entity::setPosition(const NewDouble2 &position, EntityManager &entityManager,
	const VoxelGrid &voxelGrid)
{
	this->position = position;
	entityManager.updateEntityChunk(this, voxelGrid);
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->position = Double2::Zero;
	this->dataIndex = -1;
	this->animation.reset();
}

void Entity::tick(Game &game, double dt)
{
	// Get entity animation data.
	const EntityAnimationData &animationData = [this, &game]() -> const EntityAnimationData&
	{
		const WorldData &worldData = game.getGameData().getWorldData();
		const LevelData &levelData = worldData.getActiveLevel();
		const EntityManager &entityManager = levelData.getEntityManager();
		const EntityDefinition *entityDef = entityManager.getEntityDef(this->getDataIndex());
		DebugAssert(entityDef != nullptr);

		return entityDef->getAnimationData();
	}();

	// Animate.
	auto &animation = this->getAnimation();
	animation.tick(dt, animationData);
}
