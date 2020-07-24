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
#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"

class Game;

enum class EntityType;

using EntityID = int;

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
		std::unordered_map<EntityID, int> indices;

		// List of previously-owned entity indices that can be replaced with new entities.
		std::vector<int> freeIndices;

		// Finds the next free entity index to add to, allocating if necessary.
		int nextFreeIndex();
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
		std::optional<int> getEntityIndex(EntityID id) const;

		// Inserts a new entity and assigns it the given ID.
		T *addEntity(EntityID id);

		// Moves an entity from the old group to this group.
		void acquireEntity(EntityID id, EntityGroup<T> &oldGroup);

		// Removes an entity from the group.
		void remove(EntityID id);

		// Removes all entities.
		void clear();
	};

	// One group per chunk, split into static and dynamic types.
	Buffer2D<EntityGroup<StaticEntity>> staticGroups;
	Buffer2D<EntityGroup<DynamicEntity>> dynamicGroups;

	// Entity definitions.
	std::vector<EntityDefinition> entityDefs;

	// Free IDs (previously owned) and the next available ID (never owned).
	std::vector<EntityID> freeIDs;
	EntityID nextID;

	// Obtains an available ID to be assigned to a new entity, incrementing the current max
	// if no previously owned IDs are available to reuse.
	EntityID nextFreeID();

	bool isValidChunk(const ChunkInt2 &chunk) const;
public:
	// The default ID for entities with no ID.
	static constexpr EntityID NO_ID = -1;

	// Requires the chunks per X and Z side in the voxel grid for allocating entity groups.
	void init(SNInt chunkCountX, WEInt chunkCountZ);

	// Factory functions. These assign the entity an available ID.
	StaticEntity *makeStaticEntity();
	DynamicEntity *makeDynamicEntity();

	// Gets an entity, given their ID. Returns null if no ID matches.
	Entity *get(EntityID id);
	const Entity *get(EntityID id) const;

	// Gets number of entities of the given type in the manager.
	int getCount(EntityType entityType) const;

	// Gets total number of entities in a chunk.
	int getTotalCountInChunk(const ChunkInt2 &chunk) const;

	// Gets total number of entities in the manager.
	int getTotalCount() const;

	// Gets pointers to entities of the given type. Returns number of entities written.
	int getEntities(EntityType entityType, Entity **outEntities, int outSize);
	int getEntities(EntityType entityType, const Entity **outEntities, int outSize) const;

	// Gets pointers to all entities in a chunk. Returns number of entities written.
	int getTotalEntitiesInChunk(const ChunkInt2 &chunk, const Entity **outEntities, int outSize) const;

	// Gets pointers to all entities. Returns number of entities written.
	int getTotalEntities(const Entity **outEntities, int outSize) const;

	// Gets an entity definition for the given flat index, or null if it doesn't exist.
	const EntityDefinition *getEntityDef(int flatIndex) const;

	// Adds an entity data definition to the definitions list and returns a pointer to it.
	EntityDefinition *addEntityDef(EntityDefinition &&def);

	// Gets the data necessary for rendering and ray cast selection.
	void getEntityVisibilityData(const Entity &entity, const NewDouble2 &eye2D,
		double ceilingHeight, const VoxelGrid &voxelGrid, EntityVisibilityData &outVisData) const;

	// Gets the entity's 3D bounding box. This is view-dependent!
	void getEntityBoundingBox(const Entity &entity, const EntityVisibilityData &visData,
		Double3 *outMin, Double3 *outMax) const;

	// Puts the entity into the chunk representative of their 3D position.
	void updateEntityChunk(Entity *entity, const VoxelGrid &voxelGrid);

	// Deletes an entity.
	void remove(EntityID id);

	// Deletes all entities and data in the manager.
	void clear();

	// Deletes all entities in the given chunk.
	void clearChunk(const ChunkInt2 &coord);

	// Ticks the entity manager by delta time.
	void tick(Game &game, double dt);
};

#endif
