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
using EntityCombatStateID = int;
using EntityCreatureSoundInstanceID = int;
using EntityCitizenDirectionIndexID = int;
using EntityCitizenNameID = int;
using EntityPaletteIndicesInstanceID = int;
using EntityItemInventoryInstanceID = int;
using EntityLockStateID = int;

struct EntityInstance
{
	EntityInstanceID instanceID;
	EntityDefID defID;
	EntityPositionID positionID;
	EntityBoundingBoxID bboxID;
	EntityDirectionID directionID;
	EntityAnimationInstanceID animInstID;
	EntityCombatStateID combatStateID;
	EntityCreatureSoundInstanceID creatureSoundInstID;
	EntityCitizenDirectionIndexID citizenDirectionIndexID;
	EntityCitizenNameID citizenNameID;
	EntityPaletteIndicesInstanceID paletteIndicesInstID;
	EntityItemInventoryInstanceID itemInventoryInstID;
	EntityLockStateID lockStateID;
	JPH::BodyID physicsBodyID;
	int transformHeapIndex;
	int transformIndex;

	EntityInstance();

	// All entities at least have an instance ID, definition, position, bounding box, and render transform.
	void init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID, EntityBoundingBoxID bboxID, int transformHeapIndex, int transformIndex);

	// Whether the entity is capable of moving + looking.
	bool isDynamic() const;

	bool canAcceptCombatHits() const;
	bool canBeKilledInCombat() const;

	// Whether the entity can be placed on raised platforms.
	bool canUseElevatedPlatforms() const;

	bool isCitizen() const;

	bool hasInventory() const;

	bool canBeLocked() const;

	void clear();
};

#endif
