#ifndef ENTITY_OBSERVED_RESULT_H
#define ENTITY_OBSERVED_RESULT_H

#include "EntityInstance.h"

// Animation values of an entity observed by a camera.
struct EntityObservedResult
{
	EntityInstanceID entityInstID;
	int stateIndex;
	int angleIndex;
	int keyframeIndex;
	int linearizedKeyframeIndex;

	EntityObservedResult();

	void init(EntityInstanceID entityInstID, int stateIndex, int angleIndex, int keyframeIndex, int linearizedKeyframeIndex);
};

#endif
