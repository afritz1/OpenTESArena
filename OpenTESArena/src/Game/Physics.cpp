#include <cmath>

#include "Physics.h"
#include "../Assets/MIFFile.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityType.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

#include "components/debug/Debug.h"

bool Physics::testInitialVoxelRay(const Double3 &rayStart, const Double3 &direction,
	const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
	const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
	Physics::Hit &hit)
{
	const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

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
		hit.t = (nearPoint - rayStart).length();
		hit.point = rayStart + (direction * hit.t);
		hit.voxel = voxel;
		hit.facing = facing;
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
		double nextX = static_cast<double>((direction.x >= 0) ? (voxel.x + 1) : voxel.x);
		double nextY = static_cast<double>((direction.y >= 0) ? (voxel.y + 1) : voxel.y);
		double nextZ = static_cast<double>((direction.z >= 0) ? (voxel.z + 1) : voxel.z);
		double distX = std::abs(nextX - farPoint.x);
		double distY = std::abs(nextY - (farPoint.y / ceilingHeight));
		double distZ = std::abs(nextZ - farPoint.z);

		double tX = (direction.x == 0) ? std::numeric_limits<double>::max() : (distX / std::abs(direction.x));
		double tY = (direction.y == 0) ? std::numeric_limits<double>::max() : (distY / std::abs(direction.y));
		double tZ = (direction.z == 0) ? std::numeric_limits<double>::max() : (distZ / std::abs(direction.z));

		Double3 nextPoint = farPoint;
		int stepAxis = 0;
		if (tX <= tY && tX <= tZ)
		{
			nextPoint = farPoint + (direction * tX);
		}
		else if (tY <= tX && tY <= tZ)
		{
			nextPoint = farPoint + (direction * tY);
			stepAxis = 1;
		}
		else if (tZ <= tX && tZ <= tY)
		{
			nextPoint = farPoint + (direction * tZ);
			stepAxis = 2;
		}

		const bool isRightDiag = diagData.type1;

		// Check if the next point is on an X or Z face, The Y faces are a special case
		if (nextPoint.x == std::floor(nextPoint.x) || nextPoint.z == std::floor(nextPoint.z))
		{
			// We can simplify this to a ray/wall intersection in the XZ plane
			double A = isRightDiag ? -1 : 1;
			double B = isRightDiag ? 1 : 0;

			double dzdx = -direction.z / direction.x;
			double z0 = -(farPoint.x - voxel.x) * dzdx;

			if (std::floor(farPoint.x) == farPoint.x)
			{
				z0 = farPoint.z - voxel.z;
			}

			double rayA = dzdx;
			double rayB = z0;

			double intersection = (rayB - B) / (A - rayA);
			if (intersection >= 0 && intersection <= 1)
			{
				hit.point = (nextPoint * intersection) + (farPoint * (1.0 - intersection));
				hit.t = (nearPoint - hit.point).length();
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}
		else
		{
			Double3 cornerA = Double3(isRightDiag ? 0 : 1, 0, 0);
			Double3 cornerB = Double3(isRightDiag ? 1 : 0, 0, 1);

			Double3 nextPointProjection(
				nextPoint.x - std::floor(nextPoint.x),
				0,
				nextPoint.z - std::floor(nextPoint.z));
			Double3 farPointProjection(
				farPoint.x - voxel.x,
				0,
				farPoint.z - voxel.z);

			if ((cornerA - cornerB).cross(farPointProjection - cornerB).y * (cornerA - cornerB).cross(nextPointProjection - cornerB).y <= 0)
			{
				// We already know that the point is on the diagonal plane, so we just need to do a
				// ray/plane intersection to get the point on the plane.
				MathUtils::rayPlaneIntersection(nearPoint, direction,
					Double3(voxel.x + 0.5, voxel.y + (ceilingHeight / 2), voxel.z + 0.5),
					isRightDiag ? Double3(-1, 0, 1) : Double3(1, 0, 1), hit.point);

				hit.t = (nearPoint - hit.point).length();
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}

		return false;
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Always invisible (no back face).
		// You can't click an invisible wall that you're inside of
		return false;
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		const VoxelData::EdgeData &edgeData = voxelData.edge;

		// See if the intersected facing and the edge's facing are the same, and only
		// consider edges with collision.
		const VoxelData::Facing edgeFacing = edgeData.facing;

		if ((edgeFacing == facing) && edgeData.collider)
		{
			// See if the Y offset brings the ray within the face's area.
			// @todo: ceiling height, voxel Y, etc.. Needs to be in 3D.
			hit.t = (nearPoint - rayStart).length();
			hit.point = rayStart + (direction * hit.t);
			hit.voxel = voxel;
			hit.facing = facing;
			hit.type = Hit::Type::Voxel;
			hit.voxelID = voxelID;
			return true;
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

		// @todo: for now, doors won't be clickable when in the same voxel for simplicity's sake,
		// due to various oddities (some doors have back faces, swinging doors have corner
		// preferences, etc.).
		return false;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelDataType)));
	}
}

bool Physics::testVoxelRay(const Double3 &rayStart, const Double3 &direction,
	const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
	const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

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
		hit.t = (nearPoint - farPoint).length();
		hit.point = rayStart + (direction * hit.t);
		hit.voxel = voxel;
		hit.facing = facing;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;
	}
	else if (voxelDataType == VoxelDataType::Floor)
	{
		hit.t = (nearPoint - farPoint).length();
		hit.point = rayStart + (direction * hit.t);
		hit.voxel = voxel;
		hit.facing = VoxelData::Facing::PositiveY;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;
	}
	else if (voxelDataType == VoxelDataType::Ceiling)
	{
		hit.t = (nearPoint - farPoint).length();
		hit.point = rayStart + (direction * hit.t);
		hit.voxel = voxel;
		hit.facing = VoxelData::Facing::NegativeY;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;
	}
	else if (voxelDataType == VoxelDataType::Raised)
	{
		// Check ceiling if above, wall/ceiling/floor if inside, and floor if below.
		const VoxelData::RaisedData &raisedData = voxelData.raised;

		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxel.y) * voxelHeight;

		const Double3 nearCeilingPoint(
			farPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			farPoint.z);
		const Double3 nearFloorPoint(
			farPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			farPoint.z);

		if (farPoint.y > nearCeilingPoint.y)
		{
			// We're looking from above the raised platform. We need to do 1 more step forward
			// to see if the ray crosses below the top of the raised platform by the time it
			// reaches the next voxel in the grid.
			double nextX = static_cast<double>((direction.x >= 0) ? (voxel.x + 1) : voxel.x);
			double nextZ = static_cast<double>((direction.z >= 0) ? (voxel.z + 1) : voxel.z);
			double distX = std::abs(nextX - farPoint.x);
			double distZ = std::abs(nextZ - farPoint.z);

			double tX = (direction.x == 0) ? std::numeric_limits<double>::max() : (distX / std::abs(direction.x));
			double tZ = (direction.z == 0) ? std::numeric_limits<double>::max() : (distZ / std::abs(direction.z));

			Double3 nextPoint = farPoint;
			if (tX <= tZ)
			{
				nextPoint = farPoint + (direction * tX);
			}
			else if (tZ <= tX)
			{
				nextPoint = farPoint + (direction * tZ);
			}

			if (nextPoint.y <= nearCeilingPoint.y)
			{
				const double s = (nearCeilingPoint.y - farPoint.y) / (nextPoint.y - farPoint.y);
				const Double3 hitPoint = farPoint + ((nextPoint - farPoint) * s);

				hit.t = (nearPoint - hitPoint).length();
				hit.point = rayStart + (direction * hit.t);
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}
		else if (farPoint.y < nearFloorPoint.y)
		{
			// We're looking from below the raised platform. We need to do 1 more step forward
			// to see if the ray crosses above the bottom of the raised platform by the time it
			// reaches the next voxel in the grid.
			double nextX = static_cast<double>((direction.x >= 0) ? (voxel.x + 1) : voxel.x);
			double nextZ = static_cast<double>((direction.z >= 0) ? (voxel.z + 1) : voxel.z);
			double distX = std::abs(nextX - farPoint.x);
			double distZ = std::abs(nextZ - farPoint.z);

			double tX = (direction.x == 0) ? std::numeric_limits<double>::max() : (distX / std::abs(direction.x));
			double tZ = (direction.z == 0) ? std::numeric_limits<double>::max() : (distZ / std::abs(direction.z));

			Double3 nextPoint = farPoint;
			if (tX <= tZ)
			{
				nextPoint = farPoint + (direction * tX);
			}
			else if (tZ <= tX)
			{
				nextPoint = farPoint + (direction * tZ);
			}

			if (nextPoint.y >= nearFloorPoint.y)
			{
				const double s = (nearFloorPoint.y - farPoint.y) / (nextPoint.y - farPoint.y);
				const Double3 hitPoint = farPoint + ((nextPoint - farPoint) * s);

				hit.t = (nearPoint - hitPoint).length();
				hit.point = rayStart + (direction * hit.t);
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}
		else
		{
			// The ray collided with the side of the voxel, so we know it was clicked.
			hit.t = (nearPoint - farPoint).length();
			hit.point = rayStart + (direction * hit.t);
			hit.voxel = voxel;
			hit.facing = facing;
			hit.type = Hit::Type::Voxel;
			hit.voxelID = voxelID;
			return true;
		}

		return false;
	}
	else if (voxelDataType == VoxelDataType::Diagonal)
	{
		const VoxelData::DiagonalData &diagData = voxelData.diagonal;

		// Continue the ray into the voxel and do line intersection.
		double nextX = static_cast<double>((direction.x >= 0) ? (voxel.x + 1) : voxel.x);
		double nextY = static_cast<double>((direction.y >= 0) ? (voxel.y + 1) : voxel.y);
		double nextZ = static_cast<double>((direction.z >= 0) ? (voxel.z + 1) : voxel.z);
		double distX = std::abs(nextX - farPoint.x);
		double distY = std::abs(nextY - (farPoint.y / ceilingHeight));
		double distZ = std::abs(nextZ - farPoint.z);

		double tX = (direction.x == 0) ? std::numeric_limits<double>::max() : (distX / std::abs(direction.x));
		double tY = (direction.y == 0) ? std::numeric_limits<double>::max() : (distY / std::abs(direction.y));
		double tZ = (direction.z == 0) ? std::numeric_limits<double>::max() : (distZ / std::abs(direction.z));

		Double3 nextPoint = farPoint;
		int stepAxis = 0;
		if (tX <= tY && tX <= tZ)
		{
			nextPoint = farPoint + (direction * tX);
		}
		else if (tY <= tX && tY <= tZ)
		{
			nextPoint = farPoint + (direction * tY);
			stepAxis = 1;
		}
		else if (tZ <= tX && tZ <= tY)
		{
			nextPoint = farPoint + (direction * tZ);
			stepAxis = 2;
		}

		const bool isRightDiag = diagData.type1;

		// Check if the next point is above or below. That's a special case
		if (nextPoint.x == std::floor(nextPoint.x) || nextPoint.z == std::floor(nextPoint.z))
		{
			// We can simplify this to a ray/wall intersection in the XZ plane
			double A = isRightDiag ? -1 : 1;
			double B = isRightDiag ? 1 : 0;

			double dzdx = -direction.z / direction.x;
			double z0 = -(farPoint.x - voxel.x) * dzdx;

			if (std::floor(farPoint.x) == farPoint.x)
			{
				z0 = farPoint.z - voxel.z;
			}
			
			double rayA = dzdx;
			double rayB = z0;

			double intersection = (rayB - B) / (A - rayA);
			if (intersection >= 0 && intersection <= 1)
			{
				Double3 hitPoint = (nextPoint * intersection) + (farPoint * (1.0 - intersection));
				hit.t = (nearPoint - hitPoint).length();
				hit.point = rayStart + (direction * hit.t);
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}
		else
		{
			Double3 cornerA(isRightDiag ? 0 : 1, 0, 0);
			Double3 cornerB(isRightDiag ? 1 : 0, 0, 1);

			Double3 nextPointProjection(
				nextPoint.x - std::floor(nextPoint.x),
				0,
				nextPoint.z - std::floor(nextPoint.z));
			Double3 farPointProjection(
				farPoint.x - voxel.x,
				0,
				farPoint.z - voxel.z);

			if ((cornerA - cornerB).cross(farPointProjection - cornerB).y * (cornerA - cornerB).cross(nextPointProjection - cornerB).y <= 0)
			{
				// We already know that the point is on the diagonal plane, so we just need to do a
				// ray/plane intersection to get the point on the plane.
				MathUtils::rayPlaneIntersection(nearPoint, direction,
					Double3(voxel.x + 0.5, voxel.y + (ceilingHeight / 2), voxel.z + 0.5),
					isRightDiag ? Double3(-1, 0, 1) : Double3(1, 0, 1), hit.point);

				hit.t = (nearPoint - hit.point).length();
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}

		return false;
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Only select voxels that are collidable.
		if (voxelData.transparentWall.collider)
		{
			// Check if we clicked on the side of the voxel.
			if (farPoint.x == std::floor(farPoint.x) || farPoint.z == std::floor(farPoint.z))
			{
				// Transparent walls are hit when the camera is outside of their voxel.
				hit.t = (nearPoint - farPoint).length();
				hit.point = rayStart + (direction * hit.t);
				hit.voxel = voxel;
				hit.facing = facing;
				hit.type = Hit::Type::Voxel;
				hit.voxelID = voxelID;
				return true;
			}
		}

		return false;
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		const VoxelData::EdgeData &edgeData = voxelData.edge;

		// See if the intersected facing and the edge's facing are the same, and only
		// consider edges with collision.
		const VoxelData::Facing edgeFacing = edgeData.facing;

		if ((edgeFacing == facing) && edgeData.collider)
		{
			// See if the Y offset brings the ray within the face's area.
			// @todo: ceiling height, voxel Y, etc.. Needs to be in 3D.
			hit.t = (nearPoint - rayStart).length();
			hit.point = rayStart + (direction * hit.t);
			hit.voxel = voxel;
			hit.facing = facing;
			hit.type = Hit::Type::Voxel;
			hit.voxelID = voxelID;
			return true;
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

		// @todo: ideally this method would take any hit on a door into consideration, since
		// it's the calling code's responsibility to decide what to do based on the door's open
		// state, but for now it will assume closed doors only, for simplicity.

		// Cheap/incomplete solution: assume closed, treat like a wall, always a hit.
		hit.t = (nearPoint - farPoint).length();
		hit.point = rayStart + (direction * hit.t);
		hit.voxel = voxel;
		hit.facing = facing;
		hit.type = Hit::Type::Voxel;
		hit.voxelID = voxelID;
		return true;

		/*if (doorType == VoxelData::DoorData::Type::Swinging)
		{

		}
		else if (doorType == VoxelData::DoorData::Type::Sliding)
		{

		}
		else if (doorType == VoxelData::DoorData::Type::Raising)
		{

		}
		else if (doorType == VoxelData::DoorData::Type::Splitting)
		{

		}
		else
		{
			throw DebugException("Invalid door type \"" +
				std::to_string(static_cast<int>(doorType)) + "\".");
		}*/
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelDataType)));
	}
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &direction, double ceilingHeight,
	const VoxelGrid &voxelGrid, const Double3 &cameraForward, const EntityManager &entityManager,
	const Renderer &renderer, Physics::Hit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.t = std::numeric_limits<double>::max();

	std::unordered_map<Int3, std::vector<EntityManager::EntityVisibilityData>> voxelEntityMap;

#pragma region build Voxel->Entity map

	// Get all the entities
	std::vector<const Entity*> entities(entityManager.getTotalCount());
	const int entityCount = entityManager.getTotalEntities(
		entities.data(),
		static_cast<int>(entities.size()));

	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-cameraForward.x, 0.0, -cameraForward.z).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

	const Double2 eye2D(rayStart.x, rayStart.z);
	const Double2 cameraDir(cameraForward.x, cameraForward.z);

	for (int i = 0; i < entityCount; i++)
	{
		const Entity &entity = *entities[i];

		// Skip any entities that are behind the camera or more than 10 units away.
		Double2 entityPosRelativeToEye = entity.getPosition() - eye2D;
		if (entityPosRelativeToEye.lengthSquared() > 100 ||
			cameraDir.dot(entityPosRelativeToEye) < 0.0)
			continue;

		EntityManager::EntityVisibilityData visData;
		entityManager.getEntityVisibilityData(entity, eye2D, cameraDir, ceilingHeight, voxelGrid, visData);

		// Use a bounding box to determine which voxels the entity could be in.
		// Start with a bounding cylinder.
		double radius = visData.keyframe.getWidth() / 2;
		double height = visData.keyframe.getHeight();

		// Convert the bounding cylinder to an axis-aligned bounding box.
		double minX = visData.flatPosition.x - radius;
		double maxX = visData.flatPosition.x + radius;
		double minY = visData.flatPosition.y;
		double maxY = visData.flatPosition.y + height;
		double minZ = visData.flatPosition.z - radius;
		double maxZ = visData.flatPosition.z + radius;

		// Only iterate over the specific voxels that the entity could be in (at least partially).
		// This loop should always hit at least 1 voxel.
		for (int z = static_cast<int>(std::floor(minZ)); z <= static_cast<int>(std::floor(maxZ)); z++)
		{
			for (int y = static_cast<int>(std::floor(minY)); y <= static_cast<int>(std::floor(maxY)); y++)
			{
				for (int x = static_cast<int>(std::floor(minX)); x <= static_cast<int>(std::floor(maxX)); x++)
				{
					// Create the voxel and entity list for the map
					Int3 voxel(x, y, z);
					std::vector<EntityManager::EntityVisibilityData> entityList;

					// Check if a list already exists for this voxel. If it does, then get that one
					auto iter = voxelEntityMap.find(voxel);
					if (iter != voxelEntityMap.end())
						entityList = iter->second;

					// Add the entity to the list
					entityList.push_back(visData);

					// Update the list in the map
					voxelEntityMap.insert_or_assign(voxel, entityList);
				}
			}
		}
	}

#pragma endregion build Voxel->Entity map

#pragma region Ray Cast Voxels

	const Double3 voxelReal(
		std::floor(rayStart.x),
		std::floor(rayStart.y / ceilingHeight),
		std::floor(rayStart.z));
	const Int3 voxel(
		static_cast<int>(voxelReal.x),
		static_cast<int>(voxelReal.y),
		static_cast<int>(voxelReal.z));

	const Double3 dirSquared(
		direction.x * direction.x,
		direction.y * direction.y,
		direction.z * direction.z);

	// Height (Y size) of each voxel in the voxel grid. Some levels in Arena have
	// "tall" voxels, so the voxel height must be a variable.
	const double voxelHeight = ceilingHeight;

	// A custom variable that represents the Y "floor" of the current voxel.
	const double eyeYRelativeFloor = std::floor(rayStart.y / voxelHeight) * voxelHeight;

	// Calculate delta distances along each axis. These determine how far
	// the ray has to go until the next X, Y, or Z side is hit, respectively.
	const Double3 deltaDist(
		std::sqrt(1.0 + (dirSquared.y / dirSquared.x) + (dirSquared.z / dirSquared.x)),
		std::sqrt(1.0 + (dirSquared.x / dirSquared.y) + (dirSquared.z / dirSquared.y)),
		std::sqrt(1.0 + (dirSquared.x / dirSquared.z) + (dirSquared.y / dirSquared.z)));

	// Booleans for whether a ray component is non-negative. Used with step directions 
	// and texture coordinates.
	const bool nonNegativeDirX = direction.x >= 0.0;
	const bool nonNegativeDirY = direction.y >= 0.0;
	const bool nonNegativeDirZ = direction.z >= 0.0;

	// Calculate step directions and initial side distances.
	Int3 step;
	Double3 sideDist;
	if (nonNegativeDirX)
	{
		step.x = 1;
		sideDist.x = (voxelReal.x + 1.0 - rayStart.x) * deltaDist.x;
	}
	else
	{
		step.x = -1;
		sideDist.x = (rayStart.x - voxelReal.x) * deltaDist.x;
	}

	if (nonNegativeDirY)
	{
		step.y = 1;
		sideDist.y = (eyeYRelativeFloor + voxelHeight - rayStart.y) * deltaDist.y;
	}
	else
	{
		step.y = -1;
		sideDist.y = (rayStart.y - eyeYRelativeFloor) * deltaDist.y;
	}

	if (nonNegativeDirZ)
	{
		step.z = 1;
		sideDist.z = (voxelReal.z + 1.0 - rayStart.z) * deltaDist.z;
	}
	else
	{
		step.z = -1;
		sideDist.z = (rayStart.z - voxelReal.z) * deltaDist.z;
	}

	// Make a copy of the initial side distances. They are used for the special case 
	// of the ray ending in the same voxel it started in.
	const Double3 initialSideDist = sideDist;

	// Make a copy of the step magnitudes, converted to doubles.
	const Double3 stepReal(
		static_cast<double>(step.x),
		static_cast<double>(step.y),
		static_cast<double>(step.z));

	// Get initial voxel coordinates.
	Int3 cell = voxel;

	// ID of a hit voxel. Zero (air) by default.
	uint16_t hitID = 0;

	// Axis of a hit voxel's side. X by default.
	enum class Axis { X, Y, Z };
	Axis axis = Axis::X;

	// Distance squared (in voxels) that the ray has stepped. Square roots are
	// too slow to use in the DDA loop, so this is used instead.
	// - When using variable-sized voxels, this may be calculated differently.
	double cellDistSquared = 0.0;

	// Offset values for which corner of a voxel to compare the distance 
	// squared against. The correct corner to use is important when culling
	// shapes at max view distance.
	const Double3 startCellWithOffset(
		voxelReal.x + ((1.0 + stepReal.x) / 2.0),
		eyeYRelativeFloor + (((1.0 + stepReal.y) / 2.0) * voxelHeight),
		voxelReal.z + ((1.0 + stepReal.z) / 2.0));
	const Double3 cellOffset(
		(1.0 - stepReal.x) / 2.0,
		((1.0 - stepReal.y) / 2.0) * voxelHeight,
		(1.0 - stepReal.z) / 2.0);

	// Get dimensions of the voxel grid.
	const int gridWidth = voxelGrid.getWidth();
	const int gridHeight = voxelGrid.getHeight();
	const int gridDepth = voxelGrid.getDepth();

	// Check world bounds on the start voxel. Bounds are partially recalculated 
	// for axes that the DDA loop is stepping through.
	bool voxelIsValid = (cell.x >= 0) && (cell.y >= 0) && (cell.z >= 0) &&
		(cell.x < gridWidth) && (cell.y < gridHeight) && (cell.z < gridDepth);

	// Step through the voxel grid while the current coordinate is valid and
	// the total voxel distance stepped is less than the view distance.
	// (Note that the "voxel distance" is not the same as "actual" distance.)
	const uint16_t *voxels = voxelGrid.getVoxels();

	while (voxelIsValid && (cellDistSquared < 100)) // Arbitrary value
	{
		// Get the index of the current voxel in the voxel grid.
		const int gridIndex = cell.x + (cell.y * gridWidth) + (cell.z * gridWidth * gridHeight);

#pragma region Ray Test this voxel
		// Check if the current voxel is solid.
		const uint16_t voxelID = voxels[gridIndex];

		if (voxelID > 0)
		{
			// Boolean for whether the ray ended in the same voxel it started in.
			const bool stoppedInFirstVoxel = cell == voxel;

			// Get the distance from the camera to the hit point. It is a special case
			// if the ray stopped in the first voxel.
			double distance;
			VoxelData::Facing facing = VoxelData::Facing::NegativeX;
			if (stoppedInFirstVoxel)
			{
				if ((initialSideDist.x < initialSideDist.y) &&
					(initialSideDist.x < initialSideDist.z))
				{
					distance = initialSideDist.x;
					axis = Axis::X;
				}
				else if (initialSideDist.y < initialSideDist.z)
				{
					distance = initialSideDist.y;
					axis = Axis::Y;
				}
				else
				{
					distance = initialSideDist.z;
					axis = Axis::Z;
				}
			}
			else
			{
				const size_t axisIndex = static_cast<size_t>(axis);

				// Assign to distance based on which axis was hit.
				if (axis == Axis::X)
				{
					distance = (static_cast<double>(cell.x) - rayStart.x +
						((1.0 - stepReal.x) / 2.0)) / direction.x;
					facing = nonNegativeDirX ? VoxelData::Facing::NegativeX : VoxelData::Facing::PositiveX;
				}
				else if (axis == Axis::Y)
				{
					distance = ((static_cast<double>(cell.y) * voxelHeight) - rayStart.y +
						(((1.0 - stepReal.y) / 2.0) * voxelHeight)) / direction.y;
					facing = VoxelData::Facing::NegativeZ; // TODO: There are no facing values for Y
				}
				else
				{
					distance = (static_cast<double>(cell.z) - rayStart.z +
						((1.0 - stepReal.z) / 2.0)) / direction.z;
					facing = nonNegativeDirZ ? VoxelData::Facing::NegativeZ : VoxelData::Facing::PositiveZ;
				}
			}

			Double3 rayEnd = rayStart + (direction * distance);

			hitID = voxelID;
			bool success = false;
			if (hitID > 0)
			{
				if (stoppedInFirstVoxel)
				{
					testInitialVoxelRay(rayStart, direction, cell, facing, rayStart, rayEnd,
						ceilingHeight, voxelGrid, hit);
				}
				else
				{
					testVoxelRay(rayStart, direction, cell, facing, rayStart, rayEnd,
						ceilingHeight, voxelGrid, hit);
				}
			}
		}

#pragma endregion Ray Test this voxel

#pragma region Ray Test any entities that cross this voxel

		// Check if there are any entites that cross the current voxel.
		auto iter = voxelEntityMap.find(cell);
		if (iter != voxelEntityMap.end())
		{
			// Iterate over all the entities that cross this voxel and ray test them.
			for (auto it = iter->second.begin(); it != iter->second.end(); it++)
			{
				const EntityManager::EntityVisibilityData &visData = *it;
				const Entity &entity = *visData.entity;
				const EntityData &entityData = *entityManager.getEntityData(entity.getDataIndex());

				const Double2 flatPosition2D(
					visData.flatPosition.x,
					visData.flatPosition.z);

				// Check if the flat is somewhere in front of the camera.
				const Double2 flatEyeDiff = flatPosition2D - eye2D;
				const double flatEyeDiffLen = flatEyeDiff.length();

				if (flatEyeDiffLen < hit.t)
				{
					const double flatWidth = visData.keyframe.getWidth();
					const double flatHeight = visData.keyframe.getHeight();
					const double flatHalfWidth = flatWidth * 0.50;

					// Do a ray test to see if the ray intersects.
					Double3 intersection;
					if (MathUtils::rayPlaneIntersection(rayStart, direction, visData.flatPosition,
						flatForward, intersection))
					{
						Double3 w = intersection - visData.flatPosition;

						// Get the texture coordinates.
						Double2 uv(
							0.5 - (w.dot(flatRight) / flatWidth),
							1.0 - (w.dot(flatUp) / flatHeight));

						// Verify that the UV coordinates are within a reasonable range.
						if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
						{
							// Check the texel for these coordinates and determine if it's fully transparent.
							// If so, then the ray did not REALLY intersect the entity. This will be difficult
							// since all the texture info belongs to the software renderer. I might just have
							// to add a function to the software renderer to check if a specific texture
							// coordinate is fully transparent.
							double r, g, b, a;
							renderer.getFlatTexel(uv, entityData.getFlatIndex(), visData.keyframe.getTextureID(),
								visData.anglePercent, visData.stateType, r, g, b, a);

							// If alpha == 0, then we clicked on a transparent texel.
							if (a > 0)
							{
								double distance = (intersection - rayStart).length();
								if (distance < hit.t)
								{
									hit.entityID = entity.getID();
									hit.type = Physics::Hit::Type::Entity;
									hit.point = intersection;
									hit.t = distance;
								}
							}
						}
					}
				}
			}
		}

#pragma endregion Ray Test any entities that cross this voxel

		if (hit.t != std::numeric_limits<double>::max())
		{
			break;
		}

		if ((sideDist.x < sideDist.y) && (sideDist.x < sideDist.z))
		{
			sideDist.x += deltaDist.x;
			cell.x += step.x;
			axis = Axis::X;
			voxelIsValid &= (cell.x >= 0) && (cell.x < gridWidth);
		}
		else if (sideDist.y < sideDist.z)
		{
			sideDist.y += deltaDist.y;
			cell.y += step.y;
			axis = Axis::Y;
			voxelIsValid &= (cell.y >= 0) && (cell.y < gridHeight);
		}
		else
		{
			sideDist.z += deltaDist.z;
			cell.z += step.z;
			axis = Axis::Z;
			voxelIsValid &= (cell.z >= 0) && (cell.z < gridDepth);
		}

		// Refresh how far the current cell is from the start cell, squared.
		// The "offsets" move each point to the correct corner for each voxel
		// so that the stepping stops correctly at max view distance.
		const Double3 cellDiff(
			(static_cast<double>(cell.x) + cellOffset.x) - startCellWithOffset.x,
			(static_cast<double>(cell.y) + cellOffset.y) - startCellWithOffset.y,
			(static_cast<double>(cell.z) + cellOffset.z) - startCellWithOffset.z);
		cellDistSquared = (cellDiff.x * cellDiff.x) + (cellDiff.y * cellDiff.y) +
			(cellDiff.z * cellDiff.z);
	}

#pragma endregion Ray Cast Voxels and Entities

	// Return whether the ray hit something.
	return hit.t != std::numeric_limits<double>::max();
}

bool Physics::rayCast(const Double3 &point, const Double3 &direction, const VoxelGrid &voxelGrid,
	const Double3 &cameraForward, const EntityManager &entityManager, const Renderer &renderer,
	Physics::Hit &hit)
{
	const double ceilingHeight = 1.0;
	return Physics::rayCast(point, direction, ceilingHeight, voxelGrid, cameraForward,
		entityManager, renderer, hit);
}
