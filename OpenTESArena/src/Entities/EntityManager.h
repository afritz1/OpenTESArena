#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <map>
#include <memory>
#include <vector>

class Entity;

enum class EntityType;

class EntityManager
{
private:
	std::map<int32_t, std::unique_ptr<Entity>> entities;
public:
	EntityManager();
	~EntityManager();

	// Gets an entity pointer, given their ID. Returns null if no ID matches.
	Entity *at(int32_t id) const;

	// Gets all entities of the given type.
	std::vector<Entity*> getEntities(EntityType entityType) const;

	// Obtains an available ID to be assigned to a new entity.
	int32_t nextID();
	
	// Adds an entity. The entity must get their ID from "nextID()" beforehand.
	void add(std::unique_ptr<Entity> entity);

	// Deletes an entity.
	void remove(int32_t id);
};

#endif
