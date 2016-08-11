#include <cassert>
#include <map>

#include "Voxel.h"

#include "VoxelType.h"
#include "../Math/Rect3D.h"

// These voxel type things are still experimental. I'm not sure if I'll use them in
// this exact form.

const std::map<VoxelType, std::string> VoxelTypeDisplayNames =
{

};

// Each voxel type has a set of rectangles that define its contents. These are intended 
// for rendering, but could really be used anywhere.
const std::map<VoxelType, std::vector<Rect3D>> VoxelTypeGeometries =
{

};

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

std::string Voxel::typeToString() const
{
	auto displayName = VoxelTypeDisplayNames.at(this->getVoxelType());
	return displayName;
}

std::vector<Rect3D> Voxel::getGeometry() const
{
	auto geometry = VoxelTypeGeometries.at(this->getVoxelType());
	return geometry;
}
