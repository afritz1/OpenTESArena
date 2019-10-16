#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <array>
#include <memory>
#include <vector>

#include "../Entities/Entity.h"

enum class EntityType;

class EntityManager
{
private:
	// Matches number of elements in EntityType enum.
	static constexpr int ENTITY_TYPE_COUNT = 5;

	using EntityList = std::vector<std::unique_ptr<Entity>>;

	// One list per entity type, sorted by ID.
	std::array<EntityList, ENTITY_TYPE_COUNT> entityLists;

	// Comparison function for sorting entities.
	static int entityComparer(const std::unique_ptr<Entity> &a, const std::unique_ptr<Entity> &b);

	// Gets entity list by entity type.
	EntityList &getEntityList(EntityType entityType);
	const EntityList &getEntityList(EntityType entityType) const;

	// Gets the index that the given entity should be inserted at in the sorted entity list.
	static int getEntityInsertIndex(int id, const EntityList &entityList);

	// Writes entity index to the out parameter if the entity exists in the given list.
	static bool tryGetEntityIndex(int id, const EntityList &entityList, int *outIndex);

	// Writes entity list index to the out parameter if the entity exists in any list.
	bool tryGetEntityListIndex(int id, int *outIndex) const;

	// Obtains an available ID to be assigned to a new entity.
	int getFreeID() const;
public:
	// The default ID assigned to entities that have no ID.
	static const int NO_ID;
	
	EntityManager() = default;
	EntityManager(EntityManager &&entityManager) = default;

	EntityManager &operator=(EntityManager &&entityManager) = default;

	// Gets an entity, given their ID. Returns null if no ID matches.
	Entity *get(int id) const;

	// Gets number of entities of the given type in the manager.
	int getCount(EntityType entityType) const;

	// Gets total number of entities in the manager.
	int getTotalCount() const;

	// Gets pointers to entities of the given type. Returns number of entities written.
	int getEntities(EntityType entityType, Entity **outEntities, int outSize);

	// Gets pointers to all entities. Returns number of entities written.
	int getTotalEntities(Entity **outEntities, int outSize);
	
	// Adds an entity. The entity will have their ID assigned in here.
	Entity *add(std::unique_ptr<Entity> entity);

	// Deletes an entity.
	void remove(int id);
};

#endif
