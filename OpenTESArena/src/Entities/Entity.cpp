#include <cassert>

#include "Entity.h"

#include "EntityManager.h"
#include "EntityType.h"

Entity::Entity(EntityManager &entityManager)
{
	this->id = entityManager.nextID();
}

Entity::~Entity()
{

}

int Entity::getID() const
{
	return this->id;
}
