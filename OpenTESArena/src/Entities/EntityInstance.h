#ifndef ENTITY_INSTANCE_H
#define ENTITY_INSTANCE_H

using EntityInstanceID = int;
using EntityPositionID = int;
using EntityDirectionID = int;
using EntityAnimationDefinitionID = int;
using EntityAnimationInstanceID = int;

struct EntityInstance
{
	EntityInstanceID instanceID;
	EntityPositionID positionID;
	EntityDirectionID directionID;
	EntityAnimationDefinitionID animDefID;
	EntityAnimationInstanceID animInstID;

	EntityInstance();

	// All entities at least have an instance ID and position.
	void init(EntityInstanceID instanceID, EntityPositionID positionID);

	void clear();
};

#endif
