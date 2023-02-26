#ifndef ENTITY_INSTANCE_H
#define ENTITY_INSTANCE_H

#include "EntityUtils.h"

using EntityInstanceID = int;
using EntityPositionID = int;
using EntityBoundingBoxID = int;
using EntityDirectionID = int;
using EntityAnimationInstanceID = int;
using EntityCreatureSoundInstanceID = int;
using EntityPaletteInstanceID = int;

struct EntityInstance
{
	EntityInstanceID instanceID;
	EntityDefID defID;
	EntityPositionID positionID;
	EntityBoundingBoxID bboxID;
	EntityDirectionID directionID;
	EntityAnimationInstanceID animInstID;
	EntityCreatureSoundInstanceID creatureSoundInstID;
	EntityPaletteInstanceID paletteInstID;

	EntityInstance();

	// All entities at least have an instance ID, definition, position, and bounding box.
	void init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID, EntityBoundingBoxID bboxID);

	// Whether the entity is capable of moving + looking.
	bool isDynamic() const;

	bool isCitizen() const;

	void clear();
};

#endif
