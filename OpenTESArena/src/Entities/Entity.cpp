#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"

Entity::Entity()
	: position(Double2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->animInst.reset();
}

void Entity::init(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	DebugAssert(this->id != EntityManager::NO_ID);
	this->defID = defID;
	this->animInst = animInst;
}

EntityID Entity::getID() const
{
	return this->id;
}

EntityDefID Entity::getDefinitionID() const
{
	return this->defID;
}

const NewDouble2 &Entity::getPosition() const
{
	return this->position;
}

EntityAnimationInstance &Entity::getAnimInstance()
{
	return this->animInst;
}

const EntityAnimationInstance &Entity::getAnimInstance() const
{
	return this->animInst;
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
	this->defID = EntityManager::NO_DEF_ID;
	this->position = Double2::Zero;
	this->animInst.reset();
}

void Entity::tick(Game &game, double dt)
{
	const EntityAnimationDefinition &animDef = [this, &game]() -> const EntityAnimationDefinition&
	{
		const WorldData &worldData = game.getGameData().getWorldData();
		const LevelData &levelData = worldData.getActiveLevel();
		const EntityManager &entityManager = levelData.getEntityManager();
		const EntityDefinition *entityDef = entityManager.getEntityDef(this->getDefinitionID());
		DebugAssert(entityDef != nullptr);

		// @todo: use EntityAnimationLibrary + entityDef->getAnimID() instead.
		return entityDef->getAnimDef();
	}();

	// Get current animation keyframe from instance, so we know which anim def state to get.
	const int stateIndex = this->animInst.getStateIndex();
	const EntityAnimationDefinition::State &animDefState = animDef.getState(stateIndex);

	// Animate.
	this->animInst.tick(dt, animDefState.getTotalSeconds(), animDefState.isLooping());
}
