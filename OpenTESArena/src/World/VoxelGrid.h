#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <vector>

#include "VoxelData.h"
#include "../Math/Vector2.h"

// A voxel grid is a 3D array of voxel IDs with their associated voxel definitions.

class VoxelGrid
{
private:
	std::vector<char> voxels;
	std::vector<VoxelData> voxelData;
	int width, height, depth;
public:
	VoxelGrid(int width, int height, int depth);
	~VoxelGrid();

	// Transformation methods for converting voxel coordinates from Arena's format
	// (+X west, +Z south) to the new format (+X north, +Z east).
	static Int2 arenaVoxelToNewVoxel(const Int2 &voxel, int gridWidth, int gridDepth);
	static Double2 arenaVoxelToNewVoxel(const Double2 &voxel, int gridWidth, int gridDepth);

	// Methods for obtaining the dimensions of the voxel grid.
	int getWidth() const;
	int getHeight() const;
	int getDepth() const;

	// Gets a pointer to the voxel grid data.
	char *getVoxels();
	const char *getVoxels() const;

	// Gets the voxel data associated with an ID. If the voxel ID of air is 0,
	// then pass the voxel ID minus 1 instead to get the first one.
	VoxelData &getVoxelData(int id);
	const VoxelData &getVoxelData(int id) const;

	// Adds a voxel data object and returns its assigned ID (index).
	int addVoxelData(const VoxelData &voxelData);
};

#endif
