#ifndef ENTITY_VISIBILITY_STATE_H
#define ENTITY_VISIBILITY_STATE_H

#include "EntityInstance.h"
#include "../World/Coord.h"

struct EntityVisibilityState2D
{
	EntityInstanceID entityInstID;
	CoordDouble2 flatPosition;
	int stateIndex;
	int angleIndex;
	int keyframeIndex;

	EntityVisibilityState2D();

	void init(EntityInstanceID entityInstID, const CoordDouble2 &flatPosition, int stateIndex, int angleIndex, int keyframeIndex);
};

struct EntityVisibilityState3D
{
	EntityInstanceID entityInstID;
	CoordDouble3 flatPosition;
	int stateIndex;
	int angleIndex;
	int keyframeIndex;

	EntityVisibilityState3D();

	void init(EntityInstanceID entityInstID, const CoordDouble3 &flatPosition, int stateIndex, int angleIndex, int keyframeIndex);
};

#endif
