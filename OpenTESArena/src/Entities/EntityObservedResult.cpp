#include "EntityObservedResult.h"

EntityObservedResult::EntityObservedResult()
{
	this->entityInstID = -1;
	this->stateIndex = -1;
	this->angleIndex = -1;
	this->keyframeIndex = -1;
	this->linearizedKeyframeIndex = -1;
}

void EntityObservedResult::init(EntityInstanceID entityInstID, int stateIndex, int angleIndex, int keyframeIndex,
	int linearizedKeyframeIndex)
{
	this->entityInstID = entityInstID;
	this->stateIndex = stateIndex;
	this->angleIndex = angleIndex;
	this->keyframeIndex = keyframeIndex;
	this->linearizedKeyframeIndex = linearizedKeyframeIndex;
}
