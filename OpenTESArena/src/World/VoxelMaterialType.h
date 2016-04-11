#ifndef VOXEL_MATERIAL_TYPE_H
#define VOXEL_MATERIAL_TYPE_H

// A unique identifier for each material type of voxel. Each concrete voxel type
// has a mapping to a particular material.

// A voxel material type is for calculating things like physics and sound, not 
// rendering. There isn't enough information behind a material type to allow it 
// to be rendered.

// How do these voxel materials account for partially filled voxels, like shelves?
// Simple. Just treat everything not inside the geometry as air, and everything in
// the geometry as the material. This requires using some kind of method like dot
// products with each triangle and a given point to see if it's inside the shape or 
// not, and this only works if all voxels contain only convex shapes.

// Air voxels have no collision. 
// Liquid voxels slow movement.
// Solid voxels have collision.

enum class VoxelMaterialType
{
	Air,
	Liquid,
	Solid
};

#endif
