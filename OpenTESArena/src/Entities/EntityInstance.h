#ifndef ENTITY_INSTANCE_H
#define ENTITY_INSTANCE_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "EntityUtils.h"

using EntityInstanceID = int;
using EntityPositionID = int;
using EntityBoundingBoxID = int;
using EntityDirectionID = int;
using EntityAnimationInstanceID = int;
using EntityCreatureSoundInstanceID = int;
using EntityCitizenDirectionIndexID = int;
using EntityPaletteIndicesInstanceID = int;
using EntityItemInventoryInstanceID = int;

struct EntityInstance
{
	EntityInstanceID instanceID;
	EntityDefID defID;
	EntityPositionID positionID;
	EntityBoundingBoxID bboxID;
	EntityDirectionID directionID;
	EntityAnimationInstanceID animInstID;
	EntityCreatureSoundInstanceID creatureSoundInstID;
	EntityCitizenDirectionIndexID citizenDirectionIndexID;
	EntityPaletteIndicesInstanceID paletteIndicesInstID;
	EntityItemInventoryInstanceID itemInventoryInstID;
	JPH::BodyID physicsBodyID;

	EntityInstance();

	// All entities at least have an instance ID, definition, position, and bounding box.
	void init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID, EntityBoundingBoxID bboxID);

	// Whether the entity is capable of moving + looking.
	bool isDynamic() const;

	// Whether the entity can be placed on raised platforms.
	bool canUseElevatedPlatforms() const;

	bool isCitizen() const;

	bool hasInventory() const;

	void clear();
};

#endif
