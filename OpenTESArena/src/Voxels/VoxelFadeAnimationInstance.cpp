#include <algorithm>

#include "VoxelFadeAnimationInstance.h"

VoxelFadeAnimationInstance::VoxelFadeAnimationInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->speed = 0.0;
	this->percentFaded = 0.0;
}

void VoxelFadeAnimationInstance::init(SNInt x, int y, WEInt z, double speed, double percentFaded)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->speed = speed;
	this->percentFaded = percentFaded;
}

void VoxelFadeAnimationInstance::init(SNInt x, int y, WEInt z, double speed)
{
	this->init(x, y, z, speed, 0.0);
}

bool VoxelFadeAnimationInstance::isDoneFading() const
{
	return this->percentFaded == 1.0;
}

void VoxelFadeAnimationInstance::update(double dt)
{
	if (!this->isDoneFading())
	{
		const double delta = this->speed * dt;
		this->percentFaded = std::clamp(this->percentFaded + delta, this->percentFaded, 1.0);
	}
}
