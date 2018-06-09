#include <cmath>

#include "Physics.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

bool Physics::testInitialVoxelRay(const Double3 &rayStart, const Double3 &direction, 
	const Int3 &voxel, VoxelData::Facing facing, const Double2 &nearPoint,
	const Double2 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
	Physics::Hit &hit)
{
	const uint16_t voxelID = [&voxel, &voxelGrid]()
	{
		const int voxelIndex = voxel.x + (voxel.y * voxelGrid.getWidth()) +
			(voxel.z * voxelGrid.getWidth() * voxelGrid.getHeight());
		return voxelGrid.getVoxels()[voxelIndex];
	}();

	// Get the voxel data associated with the voxel.
	const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
	const VoxelDataType voxelDataType = voxelData.dataType;

	// @todo: test voxelDataType bit mask with collisionMask before doing any more tests.
	// @todo: do intersection in 3D (maybe don't need ray-plane intersection for each one;
	//        just use the farPoint parameter).

	// Determine which type the voxel data is and run the associated calculation.
	if (voxelDataType == VoxelDataType::None)
	{
		// Do nothing.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Wall)
	{
		// Opaque walls are always hit.
		hit.t = (farPoint - nearPoint).length(); // @todo: do in 3D.
		hit.point = rayStart + (direction * hit.t);
		hit.normal = VoxelData::getNormal(facing);
		hit.voxel = voxel;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;
	}
	else if (voxelDataType == VoxelDataType::Floor)
	{
		// @todo: only possible with 3D-DDA.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Ceiling)
	{
		// @todo: only possible with 3D-DDA.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Raised)
	{
		// @todo: check ceiling if above, wall/ceiling/floor if inside, and floor if below.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Diagonal)
	{
		const VoxelData::DiagonalData &diagData = voxelData.diagonal;

		// Continue the ray into the voxel and do line intersection.
		const bool isRightDiag = diagData.type1;

		// @todo.
		return false;
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Always invisible (no back face).
		return false;
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		const VoxelData::EdgeData &edgeData = voxelData.edge;

		// See if the intersected facing and the edge's facing are the same.
		const VoxelData::Facing edgeFacing = edgeData.facing;

		if (edgeFacing == facing)
		{
			// See if the Y offset brings the ray within the face's area.
			// @todo: ceiling height, voxel Y, etc..
			return false;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Chasm)
	{
		const VoxelData::ChasmData &chasmData = voxelData.chasm;

		// See if the intersected face is visible.
		if (chasmData.faceIsVisible(facing))
		{
			// See what kind of chasm it is (this has an effect on the chasm size).
			const VoxelData::ChasmData::Type chasmType = chasmData.type;

			// @todo.
			return false;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Door)
	{
		const VoxelData::DoorData &doorData = voxelData.door;

		// See what kind of door it is.
		const VoxelData::DoorData::Type doorType = doorData.type;

		// @todo.
		return false;
	}
	else
	{
		throw DebugException("Invalid voxel data type \"" +
			std::to_string(static_cast<int>(voxelDataType)) + "\".");
	}
}

bool Physics::testVoxelRay(const Double3 &rayStart, const Double3 &direction,
	const Int3 &voxel, VoxelData::Facing facing, const Double2 &nearPoint,
	const Double2 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	const uint16_t voxelID = [&voxel, &voxelGrid]()
	{
		const int voxelIndex = voxel.x + (voxel.y * voxelGrid.getWidth()) +
			(voxel.z * voxelGrid.getWidth() * voxelGrid.getHeight());
		return voxelGrid.getVoxels()[voxelIndex];
	}();

	// Get the voxel data associated with the voxel.
	const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
	const VoxelDataType voxelDataType = voxelData.dataType;

	// @todo: test voxelDataType bit mask with collisionMask before doing any more tests.
	// @todo: do intersection in 3D (maybe don't need ray-plane intersection for each one;
	//        just use the farPoint parameter).

	// Determine which type the voxel data is and run the associated calculation.
	if (voxelDataType == VoxelDataType::None)
	{
		// Do nothing.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Wall)
	{
		// Opaque walls are always hit.
		hit.t = (farPoint - Double2(rayStart.x, rayStart.z)).length(); // @todo: do in 3D.
		hit.point = rayStart + (direction * hit.t);
		hit.normal = VoxelData::getNormal(facing);
		hit.voxel = voxel;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;
	}
	else if (voxelDataType == VoxelDataType::Floor)
	{
		// @todo: only possible with 3D-DDA.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Ceiling)
	{
		// @todo: only possible with 3D-DDA.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Raised)
	{
		// @todo: check ceiling if above, wall/ceiling/floor if inside, and floor if below.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Diagonal)
	{
		const VoxelData::DiagonalData &diagData = voxelData.diagonal;

		// Continue the ray into the voxel and do line intersection.
		const bool isRightDiag = diagData.type1;

		// @todo.
		return false;
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Always invisible (no back face).
		return false;
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		const VoxelData::EdgeData &edgeData = voxelData.edge;

		// See if the intersected facing and the edge's facing are the same.
		const VoxelData::Facing edgeFacing = edgeData.facing;

		if (edgeFacing == facing)
		{
			// See if the Y offset brings the ray within the face's area.
			// @todo: ceiling height, voxel Y, etc..
			return false;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Chasm)
	{
		const VoxelData::ChasmData &chasmData = voxelData.chasm;

		// See if the intersected face is visible.
		if (chasmData.faceIsVisible(facing))
		{
			// See what kind of chasm it is (this has an effect on the chasm size).
			const VoxelData::ChasmData::Type chasmType = chasmData.type;

			// @todo.
			return false;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Door)
	{
		const VoxelData::DoorData &doorData = voxelData.door;

		// See what kind of door it is.
		const VoxelData::DoorData::Type doorType = doorData.type;

		// @todo.
		return false;
	}
	else
	{
		throw DebugException("Invalid voxel data type \"" +
			std::to_string(static_cast<int>(voxelDataType)) + "\".");
	}
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &direction, double ceilingHeight,
	const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	const Double3 voxelReal(
		std::floor(rayStart.x),
		std::floor(rayStart.y / ceilingHeight),
		std::floor(rayStart.z));
	const Int3 voxel(
		static_cast<int>(voxelReal.x),
		static_cast<int>(voxelReal.y),
		static_cast<int>(voxelReal.z));

	const Double2 directionXZ = Double2(direction.x, direction.z).normalized();

	// @todo: implement support for actual 3D-DDA (instead of just in XZ plane).
	const Double3 directionXZ_3D(directionXZ.x, 0.0, directionXZ.y);

	const double dirX = directionXZ.x;
	const double dirZ = directionXZ.y;
	const double dirXSquared = dirX * dirX;
	const double dirZSquared = dirZ * dirZ;

	const double deltaDistX = std::sqrt(1.0 + (dirZSquared / dirXSquared));
	const double deltaDistZ = std::sqrt(1.0 + (dirXSquared / dirZSquared));

	const bool nonNegativeDirX = dirX >= 0.0;
	const bool nonNegativeDirZ = dirZ >= 0.0;

	int stepX, stepZ;
	double sideDistX, sideDistZ;
	if (nonNegativeDirX)
	{
		stepX = 1;
		sideDistX = (voxelReal.x + 1.0 - rayStart.x) * deltaDistX;
	}
	else
	{
		stepX = -1;
		sideDistX = (rayStart.x - voxelReal.x) * deltaDistX;
	}

	if (nonNegativeDirZ)
	{
		stepZ = 1;
		sideDistZ = (voxelReal.z + 1.0 - rayStart.z) * deltaDistZ;
	}
	else
	{
		stepZ = -1;
		sideDistZ = (rayStart.z - voxelReal.z) * deltaDistZ;
	}

	// The Z distance from the camera to the wall, and the X or Z normal of the intersected
	// voxel face. The first Z distance is a special case, so it's brought outside the 
	// DDA loop.
	double zDistance;
	VoxelData::Facing facing;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid =
		(voxel.x >= 0) &&
		(voxel.y >= 0) &&
		(voxel.z >= 0) &&
		(voxel.x < voxelGrid.getWidth()) &&
		(voxel.y < voxelGrid.getHeight()) &&
		(voxel.z < voxelGrid.getDepth());

	if (voxelIsValid)
	{
		// Decide how far the wall is, and which voxel face was hit.
		if (sideDistX < sideDistZ)
		{
			zDistance = sideDistX;
			facing = nonNegativeDirX ? VoxelData::Facing::NegativeX :
				VoxelData::Facing::PositiveX;
		}
		else
		{
			zDistance = sideDistZ;
			facing = nonNegativeDirZ ? VoxelData::Facing::NegativeZ :
				VoxelData::Facing::PositiveZ;
		}

		// The initial near point is at the start point.
		const Double2 initialNearPoint(rayStart.x, rayStart.z);

		// The initial far point is the voxel hit. Depending on the voxel type this may
		// or may not actually be on the side of the voxel (i.e., for diagonal walls).
		const Double2 initialFarPoint(
			rayStart.x + (dirX * zDistance),
			rayStart.z + (dirZ * zDistance));

		// Check the initial voxel.
		const bool success = Physics::testInitialVoxelRay(rayStart, directionXZ_3D, voxel,
			facing, initialNearPoint, initialFarPoint, ceilingHeight, voxelGrid, hit);

		if (success)
		{
			// The ray hit something in the first block, so it can return early.
			return true;
		}
	}
	
	// The current voxel coordinate in the DDA loop.
	Int3 cell(voxel.x, voxel.y, voxel.z);

	// Lambda for stepping to the next XZ coordinate in the grid and updating the Z
	// distance for the current edge point.
	auto doDDAStep = [&rayStart, &voxelGrid, dirX, dirZ, &sideDistX, &sideDistZ, &cell,
		&facing, &voxelIsValid, &zDistance, deltaDistX, deltaDistZ, stepX, stepZ,
		nonNegativeDirX, nonNegativeDirZ]()
	{
		if (sideDistX < sideDistZ)
		{
			sideDistX += deltaDistX;
			cell.x += stepX;
			facing = nonNegativeDirX ? VoxelData::Facing::NegativeX :
				VoxelData::Facing::PositiveX;
			voxelIsValid &= (cell.x >= 0) && (cell.x < voxelGrid.getWidth());
		}
		else
		{
			sideDistZ += deltaDistZ;
			cell.z += stepZ;
			facing = nonNegativeDirZ ? VoxelData::Facing::NegativeZ :
				VoxelData::Facing::PositiveZ;
			voxelIsValid &= (cell.z >= 0) && (cell.z < voxelGrid.getDepth());
		}

		const bool onXAxis = (facing == VoxelData::Facing::PositiveX) ||
			(facing == VoxelData::Facing::NegativeX);

		// Update the Z distance depending on which axis was stepped with.
		if (onXAxis)
		{
			zDistance = (static_cast<double>(cell.x) -
				rayStart.x + static_cast<double>((1 - stepX) / 2)) / dirX;
		}
		else
		{
			zDistance = (static_cast<double>(cell.z) -
				rayStart.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;
		}
	};

	// Step forward in the grid once to leave the initial voxel and update the Z distance.
	doDDAStep();

	// Step through the voxel grid while the current coordinate is valid and no
	// intersection has occurred.
	while (voxelIsValid)
	{
		// Store the cell coordinates, axis, and Z distance for wall rendering. The
		// loop needs to do another DDA step to calculate the far point.
		const int savedCellX = cell.x;
		const int savedCellY = cell.y;
		const int savedCellZ = cell.z;
		const VoxelData::Facing savedFacing = facing;
		const double wallDistance = zDistance;

		// Decide which voxel in the XZ plane to step to next, and update the Z distance.
		doDDAStep();

		const Double2 nearPoint(
			rayStart.x + (dirX * wallDistance),
			rayStart.z + (dirZ * wallDistance));
		const Double2 farPoint(
			rayStart.x + (dirX * zDistance),
			rayStart.z + (dirZ * zDistance));

		// Check voxel.
		const Int3 savedCell(savedCellX, savedCellY, savedCellZ);
		const bool success = Physics::testVoxelRay(rayStart, directionXZ_3D, savedCell,
			savedFacing, nearPoint, farPoint, ceilingHeight, voxelGrid, hit);

		if (success)
		{
			// The ray hit something.
			return true;
		}
	}

	// The ray exited the voxel grid without hitting anything.
	return false;
}

bool Physics::rayCast(const Double3 &point, const Double3 &direction, const VoxelGrid &voxelGrid,
	Physics::Hit &hit)
{
	const double ceilingHeight = 1.0;
	return Physics::rayCast(point, direction, ceilingHeight, voxelGrid, hit);
}
