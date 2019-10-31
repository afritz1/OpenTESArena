#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType)
	: positionXZ(Double2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->entityType = entityType;
	this->textureID = -1;
}

int Entity::getID() const
{
	return this->id;
}

EntityType Entity::getEntityType() const
{
	return this->entityType;
}

int Entity::getTextureID() const
{
	return this->textureID;
}

void Entity::setID(int id)
{
	this->id = id;
}

void Entity::setPositionXZ(const Double2 &positionXZ)
{
	this->positionXZ = positionXZ;
}

void Entity::setTextureID(int textureID)
{
	this->textureID = textureID;
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->positionXZ = Double2::Zero;
	this->textureID = -1;
}
