#include "Voxel.h"

#include "VoxelType.h"

Voxel::Voxel(VoxelType voxelType)
{
	this->voxelType = voxelType;
}

Voxel::Voxel()
	: Voxel(VoxelType::Air) { }

Voxel::~Voxel()
{

}

VoxelType Voxel::getVoxelType() const
{
	return this->voxelType;
}
