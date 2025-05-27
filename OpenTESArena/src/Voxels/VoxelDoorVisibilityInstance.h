#ifndef VOXEL_DOOR_VISIBILITY_INSTANCE_H
#define VOXEL_DOOR_VISIBILITY_INSTANCE_H

#include "VoxelFacing2D.h"
#include "../World/Coord.h"

// Door face visibility depends on adjacent air voxels and camera position.
struct VoxelDoorVisibilityInstance
{
	static constexpr int MAX_FACE_COUNT = 2;

	SNInt x;
	int y;
	WEInt z;
	VoxelFacing2D visibleFaces[MAX_FACE_COUNT];
	int visibleFaceCount;

	VoxelDoorVisibilityInstance();

	void init(SNInt x, int y, WEInt z);
	void clearVisibleFaces();
	void clear();
	void update(bool isCameraNorthInclusive, bool isCameraEastInclusive, bool isNorthValid, bool isEastValid,
		bool isSouthValid, bool isWestValid);
};

#endif
