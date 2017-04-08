#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

// Voxel data is the definition of a voxel that a voxel ID points to.
// Since there will only be a few kinds of voxel data per world, their size
// can be much larger than just a byte or two.

// Perhaps much later, when voxel destruction spells like Passwall are added, 
// more data could be added here that represents a percentage of "fade".

class VoxelData
{
public:
	int sideID, floorID, ceilingID;
	double yOffset, ySize; // Offset from bottom of voxel, and "thickness" in Y.
	double topV, bottomV; // V texture coordinates between 0.0 and 1.0.

	VoxelData(int sideID, int floorID, int ceilingID, double yOffset, double ySize,
		double topV, double bottomV);

	// Default constructor for most voxels; a Y offset of 0.0 and Y size of 1.0, 
	// with the default texture coordinates.
	VoxelData(int sideID, int floorID, int ceilingID);

	// Default constructor for a 1x1x1 voxel with all sides using the same texture.
	VoxelData(int id);

	~VoxelData();
};

#endif
