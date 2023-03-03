#ifndef ENTITY_ANIMATION_INSTANCE_H
#define ENTITY_ANIMATION_INSTANCE_H

#include "EntityAnimationDefinition.h"

// Instance-specific animation data, references a shared animation definition.
struct EntityAnimationInstance
{
	static constexpr int MAX_STATES = EntityAnimationDefinition::MAX_STATES;

	// Cached data for ease of state switching.
	double targetSecondsList[MAX_STATES];
	bool isLoopingList[MAX_STATES];

	double currentSeconds; // Updated every frame.
	double targetSeconds; // Updated when changing states.
	double progressPercent; // Updated every frame.
	int currentStateIndex; // Points into this entity's animation def.
	int stateCount;
	bool isLooping; // Updated when changing states.

	EntityAnimationInstance();

	void addState(double targetSeconds, bool isLooping);

	void setStateIndex(int index);
	void resetTime();
	void clear();
	void update(double dt);
};

#endif
