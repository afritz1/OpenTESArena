#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "DynamicEntity.h"
#include "Entity.h"
#include "EntityDefinition.h"
#include "EntityRef.h"
#include "EntityUtils.h"
#include "StaticEntity.h"
#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"

class ChunkManager;
class EntityDefinitionLibrary;
class Game;

struct EntityVisibilityState2D;
struct EntityVisibilityState3D;

enum class EntityType;

class EntityManager
{
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

		// Moves an entity from the old group to this group if able.
		bool tryAcquireEntity(EntityID id, EntityGroup<T> &oldGroup);

		// Removes an entity from the group.
		void remove(EntityID id);

		// Removes all entities.
		void clear();
	};

	// All entities for a particular chunk.
	struct EntityChunk
	{
		ChunkInt2 chunk;
		EntityGroup<StaticEntity> staticGroup;
		EntityGroup<DynamicEntity> dynamicGroup;

		void init(const ChunkInt2 &chunk);

		void clear();
	};

	// One set of entity groups per chunk, split into static and dynamic types. The chunks here are driven
	// by the chunk manager.
	std::vector<EntityChunk> entityChunks;

	// Entity definitions for the currently-active level. Their definition IDs CANNOT be assumed
	// to be zero-based because these are in addition to ones in the entity definition library.
	std::unordered_map<EntityDefID, EntityDefinition> entityDefs;

	// Free IDs (previously owned) and the next available ID (never owned).
	std::vector<EntityID> freeIDs;
	EntityID nextID;

	// Obtains an available ID to be assigned to a new entity, incrementing the current max
	// if no previously owned IDs are available to reuse.
	EntityID nextFreeID();

	// Helper functions for looking up an entity in the given group by ID.
	template <typename T>
	T *getInternal(EntityID id, EntityGroup<T> &group);
	template <typename T>
	const T *getInternal(EntityID id, const EntityGroup<T> &group) const;

	// Gets the entity chunk index if it exists.
	std::optional<int> tryGetChunkIndex(const ChunkInt2 &chunk) const;
public:
	// The default ID for entities with no ID.
	static constexpr EntityID NO_ID = -1;
	static constexpr EntityDefID NO_DEF_ID = -1;

	EntityManager();

	// Factory function. This assigns the new entity an available ID. For simplicity of initialization,
	// there must be at least one active chunk, and this entity is default-assigned to that chunk.
	EntityRef makeEntity(EntityType type);

	// Gets a raw entity handle, given their ID and an optional entity type for faster look-up.
	// Returns null if no ID matches. Does not protect against dangling pointers.
	Entity *getEntityHandle(EntityID id, EntityType type);
	const Entity *getEntityHandle(EntityID id, EntityType type) const;
	Entity *getEntityHandle(EntityID id);
	const Entity *getEntityHandle(EntityID id) const;

	// Gets an entity reference that protects against dangling pointers. Returns null entity
	// if no ID matches.
	EntityRef getEntityRef(EntityID id, EntityType type);
	ConstEntityRef getEntityRef(EntityID id, EntityType type) const;
	EntityRef getEntityRef(EntityID id);
	ConstEntityRef getEntityRef(EntityID id) const;

	// Gets the number of entities of the given type in the manager.
	int getCountOfType(EntityType entityType) const;

	// Gets total number of entities in a chunk.
	int getCountInChunk(const ChunkInt2 &chunk) const;

	// Gets total number of entities in the manager.
	int getCount() const;

	// Gets pointers to entities of the given type. Returns number of entities written.
	int getEntitiesOfType(EntityType entityType, Entity **outEntities, int outSize);
	int getEntitiesOfType(EntityType entityType, const Entity **outEntities, int outSize) const;

	// Gets pointers to all entities in a chunk. Returns number of entities written.
	int getEntitiesInChunk(const ChunkInt2 &chunk, const Entity **outEntities, int outSize) const;

	// Gets pointers to all entities. Returns number of entities written.
	int getEntities(const Entity **outEntities, int outSize) const;

	// Returns whether the entity manager is able to track entities in the given chunk.
	bool hasChunk(const ChunkInt2 &chunk) const;

	// Returns whether the given entity definition ID points to a valid definition.
	bool hasEntityDef(EntityDefID defID) const;

	// Gets an entity definition for the given ID. If the definition is not a part of the active
	// level, it will look in the definition library instead.
	const EntityDefinition &getEntityDef(EntityDefID defID,
		const EntityDefinitionLibrary &entityDefLibrary) const;

	// Adds an entity definition and returns its ID.
	EntityDefID addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &entityDefLibrary);

	// Gets the entity visibility data necessary for rendering and ray cast selection.
	void getEntityVisibilityState2D(const Entity &entity, const CoordDouble2 &eye2D,
		const ChunkManager &chunkManager, const EntityDefinitionLibrary &entityDefLibrary,
		EntityVisibilityState2D &outVisState) const;
	void getEntityVisibilityState3D(const Entity &entity, const CoordDouble2 &eye2D,
		double ceilingScale, const ChunkManager &chunkManager, const EntityDefinitionLibrary &entityDefLibrary,
		EntityVisibilityState3D &outVisState) const;

	// Convenience function for getting the active keyframe from an entity, given some
	// visibility data.
	const EntityAnimationDefinition::Keyframe &getEntityAnimKeyframe(const Entity &entity,
		const EntityVisibilityState3D &visState, const EntityDefinitionLibrary &entityDefLibrary) const;

	// Puts the entity into the chunk representative of their 3D position.
	void updateEntityChunk(Entity *entity);

	// Deletes an entity.
	void remove(EntityID id);

	// Deletes all entities and data in the manager.
	void clear();

	// Adds a new chunk entry to the manager.
	void addChunk(const ChunkInt2 &chunkPos);

	// Deletes all entities in the given chunk and removes it from the manager.
	void removeChunk(const ChunkInt2 &chunkPos);

	// Ticks the entity manager by delta time.
	void tick(Game &game, double dt);
};

#endif
