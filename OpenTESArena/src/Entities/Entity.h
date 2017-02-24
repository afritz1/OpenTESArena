#ifndef ENTITY_H
#define ENTITY_H

#include <memory>

#include "../Math/Vector3.h"

// Entities are anything in the world that isn't part of the voxel grid. Every 
// entity has a world position and a unique referencing ID. 

// Not all entities have a sprite (such as the player), and not all sprites turn to 
// face the camera (such as doors and portcullises).

class EntityManager;
class Game;

enum class EntityType;

class Entity
{
private:
	int id;
	EntityType entityType;
public:
	Entity(EntityType entityType, EntityManager &entityManager);
	virtual ~Entity();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const = 0;
	
	int getID() const;
	virtual EntityType getEntityType() const = 0;

	// Gets the 3D position of the entity. The semantics of this depends on
	// how it is implemented for each entity. For example, the player's position is
	// at their eye, while the position of NPCs is at the center of their feet.
	virtual const Double3 &getPosition() const = 0;

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt) = 0;
};

#endif
