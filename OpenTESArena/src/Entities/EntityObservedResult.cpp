#include "EntityObservedResult.h"

EntityObservedResult::EntityObservedResult()
{
	this->entityInstID = -1;
	this->linearizedKeyframeIndex = -1;
}

void EntityObservedResult::init(EntityInstanceID entityInstID, int linearizedKeyframeIndex)
{
	this->entityInstID = entityInstID;
	this->linearizedKeyframeIndex = linearizedKeyframeIndex;
}
