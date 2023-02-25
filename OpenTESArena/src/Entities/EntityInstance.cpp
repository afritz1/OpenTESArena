#include "EntityInstance.h"

#include "components/debug/Debug.h"

EntityInstance::EntityInstance()
{
	this->clear();
}

void EntityInstance::init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID)
{
	DebugAssert(instanceID >= 0);
	DebugAssert(defID >= 0);
	DebugAssert(positionID >= 0);
	this->instanceID = instanceID;
	this->defID = defID;
	this->positionID = positionID;
}

bool EntityInstance::isDynamic() const
{
	return this->directionID >= 0;
}

void EntityInstance::clear()
{
	this->instanceID = -1;
	this->defID = -1;
	this->positionID = -1;
	this->directionID = -1;
	this->animInstID = -1;
	this->creatureSoundInstID = -1;
}
