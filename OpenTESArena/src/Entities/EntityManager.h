#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "DynamicEntity.h"
#include "Entity.h"
#include "EntityDefinition.h"
#include "StaticEntity.h"
#include "../Math/Vector3.h"
#include "../World/VoxelGrid.h"

class Game;

enum class EntityType;

class EntityManager
{
public:
	struct EntityVisibilityData
	{
		const Entity *entity;
		Double3 flatPosition;
		EntityAnimationData::Keyframe keyframe;
		double anglePercent;
		EntityAnimationData::StateType stateType;

		EntityVisibilityData();
	};
private:
	template <typename T>
	class EntityGroup
	{
	private:
		static_assert(std::is_base_of_v<Entity, T>);

		// Contiguous array for fast iteration. Entries can be empty to avoid moving other
		// entries around.
		std::vector<T> entities;

		// Parallel array for whether the equivalent entities index is valid.
		std::vector<bool> validEntities;

		// Entity ID -> entity index mappings for fast insertion/deletion/look-up.
		std::unordered_map<int, int> indices;

		// List of previously-owned entity indices that can be replaced with new entities.
		std::vector<int> freeIndices;
	public:
		// Gets number of entities in the group. Intended for iterating over the entire group,
		// so it also includes any empty entries.
		int getCount() const;

		// Gets an entity by index.
		T *getEntityAtIndex(int index);
		const T *getEntityAtIndex(int index) const;

		// Helper function for entity manager getting all entities of a given type.
		int getEntities(Entity **outEntities, int outSize);
		int getEntities(const Entity **outEntities, int outSize) const;

		// Gets the index of an entity if the given ID has an associated mapping.
		std::optional<int> getEntityIndex(int id) const;

		// Inserts a new entity and assigns it the given ID.
		T *addEntity(int id);

		// Removes an entity from the group.
		void remove(int id);

		// Removes all entities.
		void clear();
	};

	// One group per entity type.
	EntityGroup<StaticEntity> staticGroup;
	EntityGroup<DynamicEntity> dynamicGroup;

	// Entity definitions.
	std::vector<EntityDefinition> entityDefs;

	// Free IDs (previously owned) and the next available ID (never owned).
	std::vector<int> freeIDs;
	int nextID;

	// Obtains an available ID to be assigned to a new entity, incrementing the current max
	// if no previously owned IDs are available to reuse.
	int nextFreeID();
public:
	// The default ID assigned to entities that have no ID.
	static const int NO_ID;
	
	EntityManager();
	EntityManager(EntityManager &&entityManager) = default;

	EntityManager &operator=(EntityManager &&entityManager) = default;

	// Factory functions. These assign the entity an available ID.
	StaticEntity *makeStaticEntity();
	DynamicEntity *makeDynamicEntity();

	// Gets an entity, given their ID. Returns null if no ID matches.
	Entity *get(int id);
	const Entity *get(int id) const;

	// Gets number of entities of the given type in the manager.
	int getCount(EntityType entityType) const;

	// Gets total number of entities in the manager.
	int getTotalCount() const;

	// Gets pointers to entities of the given type. Returns number of entities written.
	int getEntities(EntityType entityType, Entity **outEntities, int outSize);
	int getEntities(EntityType entityType, const Entity **outEntities, int outSize) const;

	// Gets pointers to all entities. Returns number of entities written.
	int getTotalEntities(const Entity **outEntities, int outSize) const;

	// Gets an entity definition for the given flat index, or null if it doesn't exist.
	const EntityDefinition *getEntityDef(int flatIndex) const;

	// Adds an entity data definition to the definitions list and returns a pointer to it.
	EntityDefinition *addEntityDef(EntityDefinition &&def);

	// Gets the data necessary for rendering and ray cast selection.
	void getEntityVisibilityData(const Entity &entity, const Double2 &eye2D, double ceilingHeight,
		const VoxelGrid &voxelGrid, EntityVisibilityData &outVisData) const;

	// Gets the entity's 3D bounding box. This is view-dependent!
	void getEntityBoundingBox(const Entity &entity, const EntityVisibilityData &visData,
		Double3 *outMin, Double3 *outMax) const;

	// Deletes an entity.
	void remove(int id);

	// Deletes all entities and data in the manager.
	void clear();

	// Ticks the entity manager by delta time.
	void tick(Game &game, double dt);
};

#endif
