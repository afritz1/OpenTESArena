#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType)
	: position(Double2::Zero)
{
	this->reset();
}

int Entity::getID() const
{
	return this->id;
}

EntityType Entity::getEntityType() const
{
	return this->entityType;
}

int Entity::getDataIndex() const
{
	return this->dataIndex;
}

const Double2 &Entity::getPosition() const
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

void Entity::init(int dataIndex)
{
	DebugAssert(this->id != EntityManager::NO_ID);
	this->dataIndex = dataIndex;
}

void Entity::setID(int id)
{
	this->id = id;
}

void Entity::setPosition(const Double2 &position)
{
	this->position = position;
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->position = Double2::Zero;
	this->textureID = -1;
	this->dataIndex = -1;
	this->animation.reset();
}
