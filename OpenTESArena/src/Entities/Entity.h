#ifndef ENTITY_H
#define ENTITY_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Entities are any objects in the world that aren't part of the voxel grid. Every entity
// has a world position and a unique referencing ID.

class Game;

enum class EntityType;

class Entity
{
private:
	int id;
	EntityType entityType;
protected:
	Double2 positionXZ;
	int textureID;
public:
	Entity(EntityType entityType);
	Entity(const Entity&) = delete;
	virtual ~Entity() = default;

	Entity &operator=(const Entity&) = delete;
	
	// Gets the unique ID for the entity.
	int getID() const;

	// Gets the entity's derived type (NPC, doodad, etc.).
	EntityType getEntityType() const;

	// @todo: rework
	// Gets the texture ID for the current animation frame, to be used by the renderer.
	int getTextureID() const;

	// @todo: rework; don't want a virtual function call for something like this.
	// Gets the 3D position of the entity. The semantics of this depends on how it is 
	// implemented for each entity. Sometimes it's an NPC's feet, sometimes it's the
	// center of a projectile.
	virtual const Double3 &getPosition() const = 0;

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt) = 0;

	// Sets the unique ID of the entity.
	void setID(int id);

	// Sets the XZ position of the entity.
	void setPositionXZ(const Double2 &positionXZ);

	// @todo: rework
	// Sets the texture ID of the entity, to be used by the renderer.
	void setTextureID(int textureID);
};

#endif
