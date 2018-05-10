#ifndef ENTITY_H
#define ENTITY_H

#include <memory>

#include "../Math/Vector3.h"

// Entities are any non-player objects in the world that aren't part of the 
// voxel grid. Every entity has a world position and a unique referencing ID.

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
	virtual ~Entity() = default;

	Entity &operator=(const Entity&) = delete;

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const = 0;
	
	// Gets the unique ID for the entity. This value is also used in the renderer to
	// find which flat is associated with the entity.
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
	// implemented for each entity. Sometimes it's an NPC's feet, sometimes it's the
	// center of a projectile.
	virtual const Double3 &getPosition() const = 0;

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt) = 0;
};

#endif
