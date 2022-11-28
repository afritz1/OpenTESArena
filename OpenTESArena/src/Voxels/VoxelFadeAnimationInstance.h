#ifndef VOXEL_FADE_ANIMATION_INSTANCE_H
#define VOXEL_FADE_ANIMATION_INSTANCE_H

#include "../World/Coord.h"

struct VoxelFadeAnimationInstance
{
	SNInt x;
	int y;
	WEInt z;
	double speed;
	double percentFaded;

	VoxelFadeAnimationInstance();

	void init(SNInt x, int y, WEInt z, double speed, double percentFaded);
	void init(SNInt x, int y, WEInt z, double speed);

	bool isDoneFading() const;

	void update(double dt);
};

#endif
