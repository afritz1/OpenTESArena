#ifndef VOXEL_TYPE_H
#define VOXEL_TYPE_H

// A unique identifier for each type of voxel. These are based on the original
// voxel types present in .MIF and .RMD files.

// Although these values might not necessarily drive behavior for a voxel, they do
// give information that would otherwise be harder to obtain (i.e., from voxel IDs).

enum class VoxelType
{
	Empty,

	// Wall types.
	Solid,
	Raised,
	Diagonal,
	TransparentWall, // 1-sided texture (i.e., wooden arches).
	TransparentEdge, // 2-sided texture (i.e., fence).
	Door,
	LevelUp,
	LevelDown,
	Unknown, // Type 0xC voxel.

	// Floor types.
	DryChasm,
	WetChasm,
	LavaChasm
};

#endif
