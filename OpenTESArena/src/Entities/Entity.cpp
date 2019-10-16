#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType)
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

void Entity::setTextureID(int textureID)
{
	this->textureID = textureID;
}
