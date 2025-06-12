#ifndef VOXEL_DOOR_ANIMATION_INSTANCE_H
#define VOXEL_DOOR_ANIMATION_INSTANCE_H

#include "../World/Coord.h"

enum class VoxelDoorAnimationStateType
{
	Closed,
	Opening,
	Open,
	Closing
};

struct VoxelDoorAnimationInstance
{
	SNInt x;
	int y;
	WEInt z;
	double speed;
	double percentOpen;
	VoxelDoorAnimationStateType stateType;

	VoxelDoorAnimationInstance();

	void init(SNInt x, int y, WEInt z, double speed, double percentOpen, VoxelDoorAnimationStateType stateType);

	// Defaults to opening so it isn't cleared on the first frame.
	void initOpening(SNInt x, int y, WEInt z, double speed);

	void setStateType(VoxelDoorAnimationStateType stateType);
	void update(double dt);
};

#endif
