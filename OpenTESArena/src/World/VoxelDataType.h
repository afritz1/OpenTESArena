#ifndef VOXEL_RENDER_TYPE_H
#define VOXEL_RENDER_TYPE_H

// A unique identifier for each type of voxel data. These are mostly used with rendering, 
// but also for determining how to interpret the voxel data itself.

// Each data type defines a way to render a voxel. Some are very similar, while others 
// are treated very differently by the renderer.

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
