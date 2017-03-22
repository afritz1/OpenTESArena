#include <algorithm>

#include "VoxelGrid.h"

VoxelGrid::VoxelGrid(int width, int height, int depth)
{
	const int voxelCount = width * height * depth;
	this->voxels = std::vector<char>(voxelCount);
	std::fill(this->voxels.begin(), this->voxels.end(), 0);

	this->width = width;
	this->height = height;
	this->depth = depth;
}

VoxelGrid::~VoxelGrid()
{

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

char *VoxelGrid::getVoxels()
{
	return this->voxels.data();
}

const char *VoxelGrid::getVoxels() const
{
	return this->voxels.data();
}

VoxelData &VoxelGrid::getVoxelData(int id)
{
	return this->voxelData.at(id);
}

const VoxelData &VoxelGrid::getVoxelData(int id) const
{
	return this->voxelData.at(id);
}

int VoxelGrid::addVoxelData(const VoxelData &voxelData)
{
	this->voxelData.push_back(voxelData);

	return static_cast<int>(this->voxelData.size() - 1);
}
