#include "VoxelData.h"

VoxelData::VoxelData(int sideID, int floorID, int ceilingID, double yOffset, 
	double ySize, double topV, double bottomV)
{
	this->sideID = sideID;
	this->floorID = floorID;
	this->ceilingID = ceilingID;
	this->diag1ID = 0;
	this->diag2ID = 0;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
}

VoxelData::VoxelData(int sideID, int floorID, int ceilingID)
	: VoxelData(sideID, floorID, ceilingID, 0.0, 1.0, 0.0, 1.0) { }

VoxelData::VoxelData(int diag1ID, int diag2ID, double yOffset, double ySize,
	double topV, double bottomV)
{
	this->sideID = 0;
	this->floorID = 0;
	this->ceilingID = 0;
	this->diag1ID = diag1ID;
	this->diag2ID = diag2ID;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
}

VoxelData::VoxelData(int diag1ID, int diag2ID)
	: VoxelData(diag1ID, diag2ID, 0.0, 1.0, 0.0, 1.0) { }

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
