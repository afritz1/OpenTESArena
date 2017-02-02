#include <cassert>

#include "Entity.h"

#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType, const Double3 &position, 
	EntityManager &entityManager)
{
	this->id = entityManager.nextID();
	this->entityType = entityType;
	this->position = position;
}

Entity::~Entity()
{

}

int Entity::getID() const
{
	return this->id;
}

const Double3 &Entity::getPosition() const
{
	return this->position;
}
