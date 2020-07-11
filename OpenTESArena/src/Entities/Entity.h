#ifndef ENTITY_H
#define ENTITY_H

#include "EntityAnimationData.h"
#include "../Math/Vector2.h"
#include "../World/VoxelUtils.h"

// Entities are any objects in the world that aren't part of the voxel grid. Every entity
// has a world position and a unique referencing ID.

class EntityManager;
class Game;
class VoxelGrid;

enum class EntityType;

class Entity
{
private:
	EntityAnimationData::Instance animation;
	int id;
	int dataIndex; // EntityDefinition index in entity manager.
protected:
	NewDouble2 position;
public:
	Entity();
	virtual ~Entity() = default;

	// Initializes the entity state (some values are initialized separately).
	void init(int dataIndex);
	
	// Gets the unique ID for the entity.
	int getID() const;

	// Gets the entity's entity manager data index.
	int getDataIndex() const;

	// Gets the XZ position of the entity.
	const NewDouble2 &getPosition() const;

	// Gets the entity's animation instance.
	EntityAnimationData::Instance &getAnimation();
	const EntityAnimationData::Instance &getAnimation() const;

	// Gets the entity's derived type (NPC, doodad, etc.).
	virtual EntityType getEntityType() const = 0;

	// Sets the entity's ID.
	void setID(int id);

	// Sets the XZ position of the entity. The entity manager needs to know about position changes.
	void setPosition(const NewDouble2 &position, EntityManager &entityManager,
		const VoxelGrid &voxelGrid);

	// Clears all entity data so it can be used for another entity of the same type.
	virtual void reset();

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt);
};

#endif
