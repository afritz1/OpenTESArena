#include <cassert>

#include "Entity.h"

#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityType entityType, const Float3d &position, 
	EntityManager &entityManager)
{
	this->id = entityManager.nextID();
	this->entityType = entityType;
	this->position = position;
}

Entity::~Entity()
{

}

int32_t Entity::getID() const
{
	return this->id;
}

const Float3d &Entity::getPosition() const
{
	return this->position;
}
