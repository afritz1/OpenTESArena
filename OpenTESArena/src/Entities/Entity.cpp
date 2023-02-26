#include <algorithm>

#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"
#include "../World/ChunkUtils.h"

Entity::Entity()
	: position(ChunkInt2::Zero, VoxelDouble2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->animInst.clear();
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

const CoordDouble2 &Entity::getPosition() const
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

void Entity::setPosition(const CoordDouble2 &position, EntityManager &entityManager)
{
	this->position = position;
	entityManager.updateEntityChunk(this);
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->position = CoordDouble2(ChunkInt2::Zero, VoxelDouble2::Zero);
	this->animInst.clear();
}

void Entity::tick(Game &game, double dt)
{
	this->animInst.update(dt);
}
