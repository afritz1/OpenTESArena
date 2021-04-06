#ifndef ENTITY_H
#define ENTITY_H

#include "EntityAnimationInstance.h"
#include "EntityUtils.h"
#include "../Math/Vector2.h"
#include "../World/VoxelUtils.h"

// Entities are any objects in the world that aren't part of the voxel grid. Every entity
// has a world position and a unique referencing ID.

class EntityDefinitionLibrary;
class EntityManager;
class Game;

enum class EntityType;

class Entity
{
private:
	EntityAnimationInstance animInst;
	EntityID id;
	EntityDefID defID;
protected:
	CoordDouble2 position;

	// Initializes the entity state (some values are initialized separately).
	void init(EntityDefID defID, const EntityAnimationInstance &animInst);
public:
	Entity();
	virtual ~Entity() = default;
	
	// Gets the unique ID for the entity.
	EntityID getID() const;

	// Gets the entity's definition ID.
	EntityDefID getDefinitionID() const;

	// Gets the chunk + point of the entity.
	const CoordDouble2 &getPosition() const;

	// Gets the entity's animation instance.
	EntityAnimationInstance &getAnimInstance();
	const EntityAnimationInstance &getAnimInstance() const;

	// Gets the entity's view-dependent and view-independent bounding boxes. The view-independent box will
	// generally be larger because it takes all animation frames into consideration.
	void getViewDependentBBox2D(const CoordDouble2 &cameraCoord, CoordDouble2 *outMin, CoordDouble2 *outMax) const;
	void getViewIndependentBBox2D(const EntityManager &entityManager, const EntityDefinitionLibrary &entityDefLibrary,
		CoordDouble2 *outMin, CoordDouble2 *outMax) const;
	void getViewIndependentBBox3D(double flatPosY, const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary, CoordDouble3 *outMin, CoordDouble3 *outMax) const;

	// Gets the entity's derived type (NPC, doodad, etc.).
	virtual EntityType getEntityType() const = 0;

	// Sets the entity's ID.
	void setID(EntityID id);

	// Sets the XZ position of the entity. The entity manager needs to know about position changes.
	void setPosition(const CoordDouble2 &position, EntityManager &entityManager);

	// Clears all entity data so it can be used for another entity of the same type.
	virtual void reset();

	// Animates the entity's state by delta time.
	virtual void tick(Game &game, double dt);
};

#endif
