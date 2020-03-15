#include <algorithm>

#include "VoxelGrid.h"

#include "components/debug/Debug.h"

VoxelGrid::VoxelGrid(int width, int height, int depth)
{
	const int voxelCount = width * height * depth;
	this->voxels = std::vector<uint16_t>(voxelCount, 0);

	this->width = width;
	this->height = height;
	this->depth = depth;
}

int VoxelGrid::getIndex(int x, int y, int z) const
{
	DebugAssert(this->coordIsValid(x, y, z));
	return x + (y * this->width) + (z * this->width * this->height);
}

int VoxelGrid::getWidth() const
{
	return this->width;
}

int VoxelGrid::getHeight() const
{
	return this->height;
}

int VoxelGrid::getDepth() const
{
	return this->depth;
}

bool VoxelGrid::coordIsValid(int x, int y, int z) const
{
	return (x >= 0) && (x < this->width) && (y >= 0) && (y < this->height) &&
		(z >= 0) && (z < this->depth);
}

uint16_t *VoxelGrid::getVoxels()
{
	return this->voxels.data();
}

const uint16_t *VoxelGrid::getVoxels() const
{
	return this->voxels.data();
}

uint16_t VoxelGrid::getVoxel(int x, int y, int z) const
{
	const int index = this->getIndex(x, y, z);
	return this->voxels.data()[index];
}

VoxelDefinition &VoxelGrid::getVoxelDef(uint16_t id)
{
	return this->voxelDefs.at(id);
}

const VoxelDefinition &VoxelGrid::getVoxelDef(uint16_t id) const
{
	return this->voxelDefs.at(id);
}

std::optional<uint16_t> VoxelGrid::findVoxelDef(const VoxelDefPredicate &predicate) const
{
	const auto iter = std::find_if(this->voxelDefs.begin(), this->voxelDefs.end(), predicate);
	if (iter != this->voxelDefs.end())
	{
		return static_cast<uint16_t>(std::distance(this->voxelDefs.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

uint16_t VoxelGrid::addVoxelDef(const VoxelDefinition &voxelDef)
{
	this->voxelDefs.push_back(voxelDef);

	return static_cast<uint16_t>(this->voxelDefs.size() - 1);
}

void VoxelGrid::setVoxel(int x, int y, int z, uint16_t id)
{
	const int index = this->getIndex(x, y, z);
	this->voxels.data()[index] = id;
}
