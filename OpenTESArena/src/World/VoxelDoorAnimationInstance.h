#ifndef VOXEL_DOOR_ANIMATION_INSTANCE_H
#define VOXEL_DOOR_ANIMATION_INSTANCE_H

#include "Coord.h"

struct VoxelDoorAnimationInstance
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

	VoxelDoorAnimationInstance();

	void init(SNInt x, int y, WEInt z, double speed, double percentOpen, StateType stateType);

	// Defaults to opening so it isn't cleared on the first frame.
	void initOpening(SNInt x, int y, WEInt z, double speed);

	void setStateType(StateType stateType);
	void update(double dt);
};

#endif
