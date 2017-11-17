#include <cassert>

#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

EntityManager::EntityManager()
{

}

EntityManager::~EntityManager()
{

}

Entity *EntityManager::at(int id) const
{
	const auto entityIter = this->entities.find(id);
	return (entityIter != this->entities.end()) ?
		entityIter->second.get() : nullptr;
}

std::vector<Entity*> EntityManager::getAllEntities() const
{
	std::vector<Entity*> entityPtrs;

	for (const auto &pair : this->entities)
	{
		Entity *entity = pair.second.get();
		entityPtrs.push_back(entity);
	}

	return entityPtrs;
}

std::vector<Entity*> EntityManager::getEntities(EntityType entityType) const
{
	std::vector<Entity*> entityPtrs;

	// Gather up entities whose type matches the given type.
	for (const auto &pair : this->entities)
	{
		Entity *entity = pair.second.get();
		if (entity->getEntityType() == entityType)
		{
			entityPtrs.push_back(entity);
		}
	}

	return entityPtrs;
}

int EntityManager::nextID() const
{
	// Iterate through IDs from 0 to infinity until one is available.
	int id = 0;
	while (this->entities.find(id) != this->entities.end())
	{
		id++;
	}

	return id;
}

void EntityManager::add(std::unique_ptr<Entity> entity)
{
	assert(entity.get() != nullptr);

	// Programmer error if two entities have the same ID.
	assert(this->entities.find(entity->getID()) == this->entities.end());

	// Add the pair to the entities map.
	int entityID = entity->getID();
	this->entities.insert(std::make_pair(entityID, std::move(entity)));
}

void EntityManager::remove(int id)
{
	this->entities.erase(id);
}
