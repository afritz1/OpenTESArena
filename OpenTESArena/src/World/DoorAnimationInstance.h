#ifndef DOOR_ANIMATION_INSTANCE_H
#define DOOR_ANIMATION_INSTANCE_H

#include "Coord.h"

struct DoorAnimationInstance
{
	enum class StateType
	{
		Closed,
		Opening,
		Open,
		Closing
	};

	SNInt x;
	int y;
	WEInt z;
	double speed;
	double percentOpen;
	StateType stateType;

	DoorAnimationInstance();

	void init(SNInt x, int y, WEInt z, double speed, double percentOpen, StateType stateType);

	// Defaults to opening so it isn't cleared on the first frame.
	void initOpening(SNInt x, int y, WEInt z, double speed);

	void setStateType(StateType stateType);
	void update(double dt);
};

#endif
