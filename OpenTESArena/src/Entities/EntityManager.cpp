#include <cassert>

#include "EntityManager.h"

#include "Entity.h"
#include "EntityType.h"

EntityManager::EntityManager()
{
	this->entities = std::map<int, std::unique_ptr<Entity>>();
}

EntityManager::~EntityManager()
{
	
}

Entity *EntityManager::at(int id) const
{
	return (this->entities.find(id) != this->entities.end()) ?
		this->entities.at(id).get() : nullptr;
}

std::vector<Entity*> EntityManager::getEntities(EntityType entityType) const
{
	std::vector<Entity*> entityPtrs;

	// Gather up entities whose type matches the given type.
	for (const auto &pair : this->entities)
	{
		if (pair.second->getEntityType() == entityType)
		{
			entityPtrs.push_back(pair.second.get());
		}
	}

	return entityPtrs;
}

int EntityManager::nextID()
{
	// Iterate through IDs from 0 to infinity until one is available.
	int id = 0;
	while (this->entities.find(id) != this->entities.end())
	{
		++id;
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
