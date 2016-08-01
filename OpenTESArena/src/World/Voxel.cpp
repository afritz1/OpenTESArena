#include <cassert>
#include <map>

#include "Voxel.h"

#include "VoxelMaterialType.h"
#include "VoxelType.h"
#include "../Math/Rect3D.h"

// These voxel type things are still experimental. I'm not sure if I'll use them in
// this exact form.

const std::map<VoxelType, std::string> VoxelTypeDisplayNames =
{

};

const std::map<VoxelMaterialType, std::string> VoxelMaterialDisplayNames =
{
	{ VoxelMaterialType::Air, "Air" },
	{ VoxelMaterialType::Liquid, "Liquid" },
	{ VoxelMaterialType::Solid, "Solid" }
};

const std::map<VoxelType, VoxelMaterialType> VoxelTypeMaterials =
{
	// Air -> Air...
	// Ground1 -> Solid...
	// etc.
};

// Each voxel type has a set of rectangles (with texture coordinates) that define
// its contents. These essentially replace "voxel templates", and are intended for 
// rendering, but could really be used anywhere.
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

VoxelMaterialType Voxel::getVoxelMaterialType() const
{
	auto materialType = VoxelTypeMaterials.at(this->getVoxelType());
	return materialType;
}

std::string Voxel::typeToString() const
{
	auto displayName = VoxelTypeDisplayNames.at(this->getVoxelType());
	return displayName;
}

std::string Voxel::materialToString() const
{
	auto displayName = VoxelMaterialDisplayNames.at(this->getVoxelMaterialType());
	return displayName;
}

std::vector<Rect3D> Voxel::getGeometry() const
{
	auto geometry = VoxelTypeGeometries.at(this->getVoxelType());
	return geometry;
}
