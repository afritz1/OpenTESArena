#ifndef ENTITY_H
#define ENTITY_H

#include <memory>

#include "../Math/Vector3.h"

// Entities are anything in the world that isn't part of the voxel grid. Every 
// entity has a world position and a unique referencing ID. 

// Not all entities have a sprite (such as the player), and not all sprites turn to 
// face the camera (such as doors and portcullises), so those ones would need to be 
// treated differently when updating the world every frame.

// --- Rendering ---
// Since the position of a light held by a sprite would likely be near its middle, 
// no shadow check between an intersection on the sprite and its light should be done, 
// in order to avoid divide-by-zero distance errors in the ray tracer. The sprite 
// would be fully lit by its own light at all points. The rectangles would probably 
// have a "hasLight" boolean that tells if it's a sprite-light, and an index into the 
// lights array to tell which one to avoid. Essentially, sprites with lights cannot 
// cast shadows from their own light, but can cast shadows from other lights, because 
// there's no zero-distance issue.

class EntityManager;
class Game;

enum class EntityType;

class Entity
{
private:
	int id;
	EntityType entityType;
protected:
	Double3 position;
public:
	Entity(EntityType entityType, const Double3 &position, EntityManager &entityManager);
	virtual ~Entity();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const = 0;
	
	int getID() const;
	const Double3 &getPosition() const;
	virtual EntityType getEntityType() const = 0;

	virtual void tick(Game *game, double dt) = 0;
};

#endif
