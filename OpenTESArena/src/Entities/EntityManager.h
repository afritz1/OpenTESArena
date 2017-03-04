#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <memory>
#include <unordered_map>
#include <vector>

class Entity;

enum class EntityType;

class EntityManager
{
private:
	std::unordered_map<int, std::unique_ptr<Entity>> entities;
public:
	EntityManager();
	EntityManager(EntityManager &&entityManager);
	~EntityManager();

	// Gets an entity pointer, given their ID. Returns null if no ID matches.
	Entity *at(int id) const;

	// Gets all entities of the given type.
	std::vector<Entity*> getEntities(EntityType entityType) const;

	// Obtains an available ID to be assigned to a new entity.
	int nextID() const;
	
	// Adds an entity. The entity must get their ID from "nextID()" beforehand.
	void add(std::unique_ptr<Entity> entity);

	// Deletes an entity.
	void remove(int id);
};

#endif
