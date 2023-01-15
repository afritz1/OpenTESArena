#ifndef ENTITY_INSTANCE_H
#define ENTITY_INSTANCE_H

#include "EntityUtils.h"

using EntityInstanceID = int;
using EntityPositionID = int;
using EntityDirectionID = int;
using EntityAnimationInstanceID = int;
using EntityCreatureSoundInstanceID = int;
using EntityPaletteInstanceID = int;

struct EntityInstance
{
	EntityInstanceID instanceID;
	EntityDefID defID;
	EntityPositionID positionID;
	EntityDirectionID directionID;
	EntityAnimationInstanceID animInstID;
	EntityCreatureSoundInstanceID creatureSoundInstID;

	EntityInstance();

	// All entities at least have an instance ID, definition, and position.
	void init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID);

	void clear();
};

#endif
