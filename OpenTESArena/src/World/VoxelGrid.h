#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <vector>

// A voxel grid is a 3D array of voxel IDs. It also has a "voxel height" value
// to accommodate some of Arena's "tall" voxels.

class VoxelGrid
{
private:
	static const double DEFAULT_VOXEL_HEIGHT;

	std::vector<char> voxels;
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

	// Gets the height of each voxel.
	double getVoxelHeight() const;

	// Gets a pointer to the voxel grid data.
	char *getVoxels();
	const char *getVoxels() const;
};

#endif
