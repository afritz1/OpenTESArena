#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <cstdint>
#include <vector>

#include "VoxelData.h"
#include "../Math/Vector2.h"

// A voxel grid is a 3D array of voxel IDs with their associated voxel definitions.

// In very complex scenes with several different kinds of voxels (including chasms, etc.),
// there are over a few hundred unique voxel data definitions, which mandates that the voxel
// type itself be at least unsigned 16-bit.

class VoxelGrid
{
private:
	std::vector<uint16_t> voxels;
	std::vector<VoxelData> voxelData;
	int width, height, depth;
public:
	VoxelGrid(int width, int height, int depth);
	~VoxelGrid();

	// Transformation methods for converting voxel coordinates between Arena's format
	// (+X west, +Z south) and the new format (+X north, +Z east). This is a bi-directional
	// conversion (i.e., it works both ways. Not exactly sure why).
	static Int2 getTransformedCoordinate(const Int2 &voxel, int gridWidth, int gridDepth);
	static Double2 getTransformedCoordinate(const Double2 &voxel, int gridWidth, int gridDepth);

	// Gets the dimensions of the voxel grid.
	int getWidth() const;
	int getHeight() const;
	int getDepth() const;

	// Gets a pointer to the voxel grid data.
	uint16_t *getVoxels();
	const uint16_t *getVoxels() const;

	// Gets the voxel data associated with an ID.
	VoxelData &getVoxelData(uint16_t id);
	const VoxelData &getVoxelData(uint16_t id) const;

	// Adds a voxel data object and returns its assigned ID.
	uint16_t addVoxelData(const VoxelData &voxelData);
};

#endif
