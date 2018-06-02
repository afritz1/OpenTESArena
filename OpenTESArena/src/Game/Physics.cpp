#include "Physics.h"
#include "../World/VoxelData.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

bool Physics::rayCast(const Double3 &point, const Double3 &direction, double ceilingHeight,
	const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	// @todo.
	return false;
}

bool Physics::rayCast(const Double3 &point, const Double3 &direction, const VoxelGrid &voxelGrid,
	Physics::Hit &hit)
{
	const double ceilingHeight = 1.0;
	return Physics::rayCast(point, direction, ceilingHeight, voxelGrid, hit);
}
