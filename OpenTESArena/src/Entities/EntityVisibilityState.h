#ifndef ENTITY_VISIBILITY_STATE_H
#define ENTITY_VISIBILITY_STATE_H

#include "../World/Coord.h"

class Entity;

struct EntityVisibilityState2D
{
	const Entity *entity;
	CoordDouble2 flatPosition;
	int stateIndex;
	int angleIndex;
	int keyframeIndex;

	EntityVisibilityState2D();

	void init(const Entity *entity, const CoordDouble2 &flatPosition, int stateIndex,
		int angleIndex, int keyframeIndex);
};

struct EntityVisibilityState3D
{
	const Entity *entity;
	CoordDouble3 flatPosition;
	int stateIndex;
	int angleIndex;
	int keyframeIndex;

	EntityVisibilityState3D();

	void init(const Entity *entity, const CoordDouble3 &flatPosition, int stateIndex,
		int angleIndex, int keyframeIndex);
};

#endif
