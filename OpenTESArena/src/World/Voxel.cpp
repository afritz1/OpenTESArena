#include <cassert>
#include <map>

#include "Voxel.h"

#include "VoxelMaterialType.h"
#include "../Math/Triangle.h"

const auto VoxelTypeDisplayNames = std::map<VoxelType, std::string>
{

};

const auto VoxelMaterialDisplayNames = std::map<VoxelMaterialType, std::string>
{
	{ VoxelMaterialType::Air, "Air" },
	{ VoxelMaterialType::Liquid, "Liquid" },
	{ VoxelMaterialType::Solid, "Solid" }
};

const auto VoxelTypeMaterials = std::map<VoxelType, VoxelMaterialType>
{
	// Air -> Air...
	// Ground1 -> Solid...
	// etc.
};

// Each voxel type has a set of triangles (with texture coordinates) that define
// its contents. These essentially replace "voxel templates", and are intended for 
// rendering, but could really be used anywhere.
const auto VoxelTypeGeometries = std::map<VoxelType, std::vector<Triangle>>
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

const VoxelType &Voxel::getVoxelType() const
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
	assert(displayName.size() > 0);
	return displayName;
}

std::string Voxel::materialToString() const
{
	auto displayName = VoxelMaterialDisplayNames.at(this->getVoxelMaterialType());
	assert(displayName.size() > 0);
	return displayName;
}

std::vector<Triangle> Voxel::getGeometry() const
{
	auto geometry = VoxelTypeGeometries.at(this->getVoxelType());
	return geometry;
}
