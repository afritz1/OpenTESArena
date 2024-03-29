#include "VoxelChasmWallInstance.h"

VoxelChasmWallInstance::VoxelChasmWallInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->north = false;
	this->east = false;
	this->south = false;
	this->west = false;
}

void VoxelChasmWallInstance::init(SNInt x, int y, WEInt z, bool north, bool east, bool south, bool west)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->north = north;
	this->east = east;
	this->south = south;
	this->west = west;
}

int VoxelChasmWallInstance::getFaceCount() const
{
	return (this->north ? 1 : 0) + (this->east ? 1 : 0) + (this->south ? 1 : 0) + (this->west ? 1 : 0);
}
