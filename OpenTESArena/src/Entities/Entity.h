#ifndef ENTITY_H
#define ENTITY_H

#include <memory>

#include "../Math/Vector3.h"

// Entities are any non-player objects in the world that aren't part of the 
// voxel grid. Every entity has a world position and a unique referencing ID. 

// Not all sprites turn to face the camera (such as doors and portcullises).

class EntityManager;
class Game;

enum class EntityType;

class Entity
{
private:
	int id;
protected:
	// Texture ID and flip state are updated by the derived entity's tick() method.
	int textureID;
	bool flipped;
public:
	Entity(EntityManager &entityManager);
	Entity(const Entity&) = delete;
	virtual ~Entity();

	Entity &operator=(const Entity&) = delete;

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const = 0;
	
	int getID() const;

	// Gets the texture ID for the current animation frame, to be used by the software 
	// renderer. This is updated through the tick() method and might also be a function 
	// of the player's position.
	int getTextureID() const;

	// Returns whether the entity's texture is currently mirrored across the Y axis. 
	// This is potentially true for entities whose texture ID depends on their orientation 
	// to the player.
	bool getFlipped() const;

	virtual EntityType getEntityType() const = 0;

	// Gets the 3D position of the entity. The semantics of this depends on how it is 
	// implemented for each entity. For example, the player's position is at their eye, 
	// while the position of NPCs is at the center of their feet.
	virtual const Double3 &getPosition() const = 0;

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt) = 0;
};

#endif
