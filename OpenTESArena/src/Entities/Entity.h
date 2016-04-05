#ifndef ENTITY_H
#define ENTITY_H

#include "EntityType.h"
#include "../Math/Float3.h"

// Entities are anything in the world that isn't part of the voxel grid. Every 
// entity has a world position and a unique referencing ID (like an integer). 
// Not all entities have a sprite (such as the player or doors), and so they would 
// be either treated differently or ignored altogether when rendering.

// There shouldn't be a "Projectile" derived class. Too much shared functionality
// with other physics-related things. It could be from something like "Moveable".

// There should be a "Light" member somewhere in the Entity hierarchy. It would 
// primarily be for static torches and things, but for people, if they have something 
// equipped that emits light, like an enchanted ring, then make that part of the 
// light list that they have. Unequipped items shouldn't emit light. They're "covered 
// by the backpack".

class Entity
{
public:
	Entity(); // Acquire ID from given EntityManager instance. No static manager classes!
	virtual ~Entity();

	// std::unique_ptr<Entity> clone()...

	// Hmm... an entity is abstract, but I want all entities to have a position and ID...
	virtual const Float3d &getPosition() const = 0;
	virtual const EntityType &getEntityType() const = 0;
	virtual const int &getID() const = 0;
};

#endif