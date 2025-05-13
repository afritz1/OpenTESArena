#include "EntityInstance.h"

#include "components/debug/Debug.h"

EntityInstance::EntityInstance()
{
	this->clear();
}

void EntityInstance::init(EntityInstanceID instanceID, EntityDefID defID, EntityPositionID positionID, EntityBoundingBoxID bboxID, UniformBufferID renderTransformBufferID)
{
	DebugAssert(instanceID >= 0);
	DebugAssert(defID >= 0);
	DebugAssert(positionID >= 0);
	DebugAssert(bboxID >= 0);
	DebugAssert(renderTransformBufferID >= 0);
	this->instanceID = instanceID;
	this->defID = defID;
	this->positionID = positionID;
	this->bboxID = bboxID;
	this->renderTransformBufferID = renderTransformBufferID;
}

bool EntityInstance::isDynamic() const
{
	return this->directionID >= 0;
}

bool EntityInstance::canAcceptCombatHits() const
{
	return this->canBeKilledInCombat() || this->canBeLocked();
}

bool EntityInstance::canBeKilledInCombat() const
{
	return this->combatStateID >= 0;
}

bool EntityInstance::canUseElevatedPlatforms() const
{
	return !this->isDynamic();
}

bool EntityInstance::isCitizen() const
{
	return this->citizenDirectionIndexID >= 0;
}

bool EntityInstance::hasInventory() const
{
	return this->itemInventoryInstID >= 0;
}

bool EntityInstance::canBeLocked() const
{
	return this->lockStateID >= 0;
}

void EntityInstance::clear()
{
	this->instanceID = -1;
	this->defID = -1;
	this->positionID = -1;
	this->bboxID = -1;
	this->directionID = -1;
	this->animInstID = -1;
	this->combatStateID = -1;
	this->creatureSoundInstID = -1;
	this->citizenDirectionIndexID = -1;
	this->citizenNameID = -1;
	this->paletteIndicesInstID = -1;
	this->itemInventoryInstID = -1;
	this->lockStateID = -1;
	this->physicsBodyID = JPH::BodyID();
	this->renderTransformBufferID = -1;
}
