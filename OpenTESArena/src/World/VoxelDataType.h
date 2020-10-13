#ifndef VOXEL_DATA_TYPE_H
#define VOXEL_DATA_TYPE_H

// A unique identifier for each type of voxel data. These are mostly used with rendering, 
// but also for determining how to interpret the voxel data itself.

// If the type is "None", then the voxel is empty and there is nothing to render.

enum class VoxelDataType
{
	None,
	Wall,
	Floor,
	Ceiling,
	Raised,
	Diagonal,
	TransparentWall,
	Edge,
	Chasm,
	Door
};

#endif
