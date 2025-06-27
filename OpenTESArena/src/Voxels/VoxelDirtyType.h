#ifndef VOXEL_DIRTY_TYPE_H
#define VOXEL_DIRTY_TYPE_H

#include <cstdint>

enum class VoxelDirtyType : uint8_t
{
	ShapeDefinition = (1 << 0), // The shape/mesh definition changed.
	FaceActivation = (1 << 1), // Face(s) changed enable/disable state, likely from neighbor shape change. Also used with chasm walls.
	DoorAnimation = (1 << 2), // Door animated this frame.
	DoorVisibility = (1 << 3), // Door visible faces changed due to camera motion.
	FadeAnimation = (1 << 4) // Fade intensity changed.
};

#endif
