#include <cassert>

#include "Entity.h"

#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType, EntityManager &entityManager)
{
	this->id = entityManager.nextID();
	this->entityType = entityType;
}

Entity::~Entity()
{

}

int Entity::getID() const
{
	return this->id;
}
