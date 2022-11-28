#include "VoxelTriggerInstance.h"

VoxelTriggerInstance::VoxelTriggerInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
}

void VoxelTriggerInstance::init(SNInt x, int y, WEInt z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}
