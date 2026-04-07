#pragma once

#include "EntityInstance.h"

// Animation values of an entity observed by a camera.
struct EntityObservedResult
{
	EntityInstanceID entityInstID;
	int linearizedKeyframeIndex;

	EntityObservedResult();

	void init(EntityInstanceID entityInstID, int linearizedKeyframeIndex);
};
