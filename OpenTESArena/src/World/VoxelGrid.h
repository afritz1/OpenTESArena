#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <vector>

#include "VoxelData.h"

// A voxel grid is a 3D array of voxel IDs with their associated voxel definitions.
// It also has a "voxel height" value to accommodate some of Arena's "tall" voxels.

class VoxelGrid
{
private:
	static const double DEFAULT_VOXEL_HEIGHT;

	std::vector<char> voxels;
	std::vector<VoxelData> voxelData;
	int width, height, depth;
	double voxelHeight; // No need for voxel width or depth; always 1.
public:
	VoxelGrid(int width, int height, int depth, double voxelHeight);
	VoxelGrid(int width, int height, int depth);
	~VoxelGrid();

	// Methods for obtaining the dimensions of the voxel grid.
	int getWidth() const;
	int getHeight() const;
	int getDepth() const;

	// Gets the height (Y size) of each voxel.
	double getVoxelHeight() const;

	// Gets a pointer to the voxel grid data.
	char *getVoxels();
	const char *getVoxels() const;

	// Gets the voxel data associated with an ID. If the voxel ID of air is 0,
	// then pass the voxel ID minus 1 instead to get the first one.
	VoxelData &getVoxelData(int index);
	const VoxelData &getVoxelData(int index) const;

	// Adds a voxel data object and returns its assigned ID (index).
	int addVoxelData(const VoxelData &voxelData);
};

#endif
