#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

// Voxel data is the definition of a voxel that a voxel ID points to.
// Since there will only be a few kinds of voxel data per world, their size
// can be much larger than just a byte or two.

// A voxel is "air" if all of its IDs are zero.

// Perhaps much later, when voxel destruction spells like Passwall are added, 
// more data could be added here that represents a percentage of "fade".

// "Diagonal 1" starts at (nearX, nearZ) and ends at (farX, farZ). "Diagonal 2"
// starts at (farX, nearZ) and ends at (nearX, farZ). The line equations for each
// are z = x and z = -x + 1, respectively.

class VoxelData
{
public:
	int sideID, floorID, ceilingID, diag1ID, diag2ID;
	double yOffset, ySize; // Offset from bottom of voxel, and "thickness" in Y.
	double topV, bottomV; // V texture coordinates between 0.0 and 1.0.

	VoxelData(int sideID, int floorID, int ceilingID, double yOffset, double ySize,
		double topV, double bottomV);

	// Default constructor for most voxels; a Y offset of 0.0 and Y size of 1.0, 
	// with the default texture coordinates.
	VoxelData(int sideID, int floorID, int ceilingID);

	VoxelData(int diag1ID, int diag2ID, double yOffset, double ySize,
		double topV, double bottomV);

	// Diagonal wall constructor. Only one of the IDs should be non-zero.
	VoxelData(int diag1ID, int diag2ID);

	// Default constructor for a 1x1x1 voxel with all sides using the same texture.
	VoxelData(int id);

	~VoxelData();

	// Returns whether all of the voxel's sides have an ID of zero.
	bool isAir() const;
};

#endif
