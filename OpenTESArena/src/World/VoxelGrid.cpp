#include <algorithm>

#include "VoxelGrid.h"

const double VoxelGrid::DEFAULT_VOXEL_HEIGHT = 1.0;

VoxelGrid::VoxelGrid(int width, int height, int depth, double voxelHeight)
{
	const int voxelCount = width * height * depth;
	this->voxels = std::vector<char>(voxelCount);
	std::fill(this->voxels.begin(), this->voxels.end(), 0);

	this->width = width;
	this->height = height;
	this->depth = depth;
	this->voxelHeight = voxelHeight;
}

VoxelGrid::VoxelGrid(int width, int height, int depth)
	: VoxelGrid(width, height, depth, VoxelGrid::DEFAULT_VOXEL_HEIGHT) { }

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

double VoxelGrid::getVoxelHeight() const
{
	return this->voxelHeight;
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
