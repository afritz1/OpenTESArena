#include "VoxelData.h"

VoxelData::VoxelData(int sideID, int floorID, int ceilingID, double yOffset, 
	double ySize, double topV, double bottomV)
{
	this->sideID = sideID;
	this->floorID = floorID;
	this->ceilingID = ceilingID;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
}

VoxelData::VoxelData(int sideID, int floorID, int ceilingID)
	: VoxelData(sideID, floorID, ceilingID, 0.0, 1.0, 0.0, 1.0) { }

VoxelData::VoxelData(int id)
	: VoxelData(id, id, id, 0.0, 1.0, 0.0, 1.0) { }

VoxelData::~VoxelData()
{

}

bool VoxelData::isAir() const
{
	return (this->sideID == 0) && (this->floorID == 0) &&
		(this->ceilingID == 0);
}
