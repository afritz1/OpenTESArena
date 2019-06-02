#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityManager &entityManager)
{
	this->id = entityManager.nextID();
	this->textureID = 0;
	this->flipped = false;
}

int Entity::getID() const
{
	return this->id;
}

int Entity::getTextureID() const
{
	return this->textureID;
}

bool Entity::getFlipped() const
{
	return this->flipped;
}
