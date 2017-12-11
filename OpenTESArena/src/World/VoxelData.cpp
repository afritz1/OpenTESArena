#include "VoxelData.h"
#include "VoxelType.h"
#include "../Utilities/Debug.h"

const int VoxelData::TOTAL_IDS = 64;

VoxelData::VoxelData(int sideID, int floorID, int ceilingID, double yOffset, 
	double ySize, double topV, double bottomV, VoxelType type)
{
	if (sideID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	this->sideID = sideID % VoxelData::TOTAL_IDS;
	this->floorID = floorID % VoxelData::TOTAL_IDS;
	this->ceilingID = ceilingID % VoxelData::TOTAL_IDS;
	this->diag1ID = 0;
	this->diag2ID = 0;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
	this->type = type;
}

VoxelData::VoxelData(int sideID, int floorID, int ceilingID, VoxelType type)
	: VoxelData(sideID, floorID, ceilingID, 0.0, 1.0, 0.0, 1.0, type) { }

VoxelData::VoxelData(int diag1ID, int diag2ID, double yOffset, double ySize,
	double topV, double bottomV, VoxelType type)
{
	if (diag1ID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Diag1 ID \"" + std::to_string(diag1ID) + "\" out of range.");
	}

	if (diag2ID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Diag2 ID \"" + std::to_string(diag2ID) + "\" out of range.");
	}

	this->sideID = 0;
	this->floorID = 0;
	this->ceilingID = 0;
	this->diag1ID = diag1ID % VoxelData::TOTAL_IDS;
	this->diag2ID = diag2ID % VoxelData::TOTAL_IDS;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->topV = topV;
	this->bottomV = bottomV;
	this->type = type;
}

VoxelData::VoxelData(int diag1ID, int diag2ID, VoxelType type)
	: VoxelData(diag1ID, diag2ID, 0.0, 1.0, 0.0, 1.0, type) { }

VoxelData::VoxelData(int id, VoxelType type)
	: VoxelData(id, id, id, 0.0, 1.0, 0.0, 1.0, type) { }

VoxelData::~VoxelData()
{

}

bool VoxelData::isAir() const
{
	return (this->sideID == 0) && (this->floorID == 0) &&
		(this->ceilingID == 0);
}
