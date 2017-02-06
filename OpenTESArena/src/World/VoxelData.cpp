#include "VoxelData.h"

VoxelData::VoxelData(int sideID, int floorAndCeilingID, double yOffset, double ySize,
	double topV, double bottomV)
{
	this->sideID = sideID;
	this->floorAndCeilingID = floorAndCeilingID;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
}

VoxelData::VoxelData(int sideID, int floorAndCeilingID)
	: VoxelData(sideID, floorAndCeilingID, 0.0, 1.0, 0.0, 1.0) { }

VoxelData::~VoxelData()
{

}
