#include "EntityInstance.h"

#include "components/debug/Debug.h"

EntityInstance::EntityInstance()
{
	this->clear();
}

void EntityInstance::init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID, EntityBoundingBoxID bboxID)
{
	DebugAssert(instanceID >= 0);
	DebugAssert(defID >= 0);
	DebugAssert(positionID >= 0);
	DebugAssert(bboxID >= 0);
	this->instanceID = instanceID;
	this->defID = defID;
	this->positionID = positionID;
	this->bboxID = bboxID;
}

bool EntityInstance::isDynamic() const
{
	return this->directionID >= 0;
}

bool EntityInstance::isCitizen() const
{
	return this->citizenDirectionIndexID >= 0;
}

void EntityInstance::clear()
{
	this->instanceID = -1;
	this->defID = -1;
	this->positionID = -1;
	this->bboxID = -1;
	this->directionID = -1;
	this->animInstID = -1;
	this->creatureSoundInstID = -1;
	this->citizenDirectionIndexID = -1;
	this->paletteIndicesInstID = -1;
	this->physicsBodyID = JPH::BodyID();
}
