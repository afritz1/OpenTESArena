#include <cmath>

#include "Physics.h"
#include "../Assets/MIFFile.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityType.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelFacing.h"
#include "../World/VoxelGeometry.h"
#include "../World/VoxelGrid.h"

#include "components/debug/Debug.h"

// @todo: allow hits on the insides of voxels until the renderer uses back-face culling (if ever).

namespace
{
	constexpr double MAX_ENTITY_DIST = 10.0;

	// Converts the normal to the associated voxel facing on success. Not all conversions
	// exist, for example, diagonals have normals but do not have a voxel facing.
	bool TryGetFacingFromNormal(const Double3 &normal, VoxelFacing *outFacing)
	{
		DebugAssert(outFacing != nullptr);

		constexpr double oneMinusEpsilon = 1.0 - Constants::Epsilon;

		bool success = true;
		if (normal.dot(Double3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::PositiveX;
		}
		else if (normal.dot(-Double3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::NegativeX;
		}
		else if (normal.dot(Double3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::PositiveY;
		}
		else if (normal.dot(-Double3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::NegativeY;
		}
		else if (normal.dot(Double3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::PositiveZ;
		}
		else if (normal.dot(-Double3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing::NegativeZ;
		}
		else
		{
			success = false;
		}

		return success;
	}
}

const double Physics::Hit::MAX_T = std::numeric_limits<double>::infinity();

void Physics::Hit::initVoxel(double t, const Double3 &point, uint16_t id, const Int3 &voxel,
	const VoxelFacing *facing)
{
	this->t = t;
	this->point = point;
	this->type = Hit::Type::Voxel;
	this->voxelHit.id = id;
	this->voxelHit.voxel = voxel;

	if (facing != nullptr)
	{
		this->voxelHit.facing = *facing;
	}
	else
	{
		this->voxelHit.facing = std::nullopt;
	}
}

void Physics::Hit::initEntity(double t, const Double3 &point, int id)
{
	this->t = t;
	this->point = point;
	this->type = Hit::Type::Entity;
	this->entityHit.id = id;
}

double Physics::Hit::getT() const
{
	return this->t;
}

double Physics::Hit::getTSqr() const
{
	return this->t * this->t;
}

const Double3 &Physics::Hit::getPoint() const
{
	return this->point;
}

Physics::Hit::Type Physics::Hit::getType() const
{
	return this->type;
}

const Physics::Hit::VoxelHit &Physics::Hit::getVoxelHit() const
{
	DebugAssert(this->getType() == Hit::Type::Voxel);
	return this->voxelHit;
}

const Physics::Hit::EntityHit &Physics::Hit::getEntityHit() const
{
	DebugAssert(this->getType() == Hit::Type::Entity);
	return this->entityHit;
}

void Physics::Hit::setT(double t)
{
	this->t = t;
}

Physics::VoxelEntityMap Physics::makeVoxelEntityMap(const Double3 &cameraPosition,
	const Double3 &cameraDirection, double ceilingHeight, const VoxelGrid &voxelGrid,
	const EntityManager &entityManager)
{
	// Get all the entities.
	std::vector<const Entity*> entities(entityManager.getTotalCount());
	const int entityCount = entityManager.getTotalEntities(
		entities.data(), static_cast<int>(entities.size()));

	const Double2 cameraPosXZ(cameraPosition.x, cameraPosition.z);
	const Double2 cameraDirXZ(cameraDirection.x, cameraDirection.z);

	// Build mappings of voxels to entities.
	VoxelEntityMap voxelEntityMap;
	for (const Entity *entityPtr : entities)
	{
		const Entity &entity = *entityPtr;

		// Skip any entities that are behind the camera or are too far away.
		Double2 entityPosEyeDiff = entity.getPosition() - cameraPosXZ;
		if (entityPosEyeDiff.lengthSquared() > (MAX_ENTITY_DIST * MAX_ENTITY_DIST) ||
			cameraDirXZ.dot(entityPosEyeDiff) < 0.0)
		{
			continue;
		}

		EntityManager::EntityVisibilityData visData;
		entityManager.getEntityVisibilityData(entity, cameraPosXZ, cameraDirXZ, ceilingHeight, voxelGrid, visData);

		// Use a bounding box to determine which voxels the entity could be in.
		// Start with a bounding cylinder.
		const double radius = visData.keyframe.getWidth() / 2.0;
		const double height = visData.keyframe.getHeight();

		// Convert the bounding cylinder to an axis-aligned bounding box.
		const double minX = visData.flatPosition.x - radius;
		const double maxX = visData.flatPosition.x + radius;
		const double minY = visData.flatPosition.y;
		const double maxY = visData.flatPosition.y + height;
		const double minZ = visData.flatPosition.z - radius;
		const double maxZ = visData.flatPosition.z + radius;

		// Only iterate over voxels the entity could be in (at least partially).
		// This loop should always hit at least 1 voxel.
		const int startX = static_cast<int>(std::floor(minX));
		const int endX = static_cast<int>(std::floor(maxX));
		const int startY = static_cast<int>(std::floor(minY / ceilingHeight));
		const int endY = static_cast<int>(std::floor(maxY / ceilingHeight));
		const int startZ = static_cast<int>(std::floor(minZ));
		const int endZ = static_cast<int>(std::floor(maxZ));

		for (int z = startZ; z <= endZ; z++)
		{
			for (int y = startY; y <= endY; y++)
			{
				for (int x = startX; x <= endX; x++)
				{
					const Int3 voxel(x, y, z);

					// Add the entity to the list. Create a new voxel->entity list mapping if
					// there isn't one for this voxel.
					auto iter = voxelEntityMap.find(voxel);
					if (iter == voxelEntityMap.end())
					{
						iter = voxelEntityMap.insert(std::make_pair(
							voxel, std::vector<EntityManager::EntityVisibilityData>())).first;
					}

					auto &entityDataList = iter->second;
					entityDataList.push_back(visData);
				}
			}
		}
	}

	return voxelEntityMap;
}

bool Physics::testInitialVoxelRay(const Double3 &rayStart, const Double3 &rayDirection,
	const Int3 &voxel, VoxelFacing farFacing, const Double3 &farPoint, double ceilingHeight,
	const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

	// Get the voxel data associated with the voxel.
	const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
	const VoxelDataType voxelDataType = voxelData.dataType;

	// Determine which type the voxel data is and run the associated calculation.
	if (voxelDataType == VoxelDataType::None)
	{
		// Do nothing.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Wall)
	{
		// Opaque walls are always hit.
		const double t = (farPoint - rayStart).length();
		const Double3 hitPoint = farPoint;
		hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
		return true;
	}
	else if (voxelDataType == VoxelDataType::Floor)
	{
		// Check if the ray hits the top of the voxel.
		if (farFacing == VoxelFacing::PositiveY)
		{
			const double t = (farPoint - rayStart).length();
			const Double3 hitPoint = farPoint;
			hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Ceiling)
	{
		// Check if the ray hits the bottom of the voxel.
		if (farFacing == VoxelFacing::NegativeY)
		{
			const double t = (farPoint - rayStart).length();
			const Double3 hitPoint = farPoint;
			hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Raised)
	{
		const VoxelData::RaisedData &raised = voxelData.raised;
		const double raisedYBottom = (static_cast<double>(voxel.y) + raised.yOffset) * ceilingHeight;
		const double raisedYTop = raisedYBottom + (raised.ySize * ceilingHeight);

		if ((rayStart.y > raisedYBottom) && (rayStart.y < raisedYTop))
		{
			// Inside the raised platform. See where the far point is.
			if (farPoint.y < raisedYBottom)
			{
				// Hits the inside floor of the raised platform.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					raisedYBottom,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::NegativeY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else if (farPoint.y > raisedYTop)
			{
				// Hits the inside ceiling of the raised platform.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					raisedYTop,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = -Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::PositiveY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else
			{
				// Hits the inside wall of the raised platform.
				const double t = (farPoint - rayStart).length();
				const Double3 hitPoint = farPoint;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
				return true;
			}
		}
		else if (rayStart.y > raisedYTop)
		{
			// Above the raised platform. See if the ray hits the top.
			if (farPoint.y <= raisedYTop)
			{
				// Hits the top somewhere.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					raisedYTop,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::PositiveY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (rayStart.y < raisedYBottom)
		{
			// Below the raised platform. See if the ray hits the bottom.
			if (farPoint.y >= raisedYBottom)
			{
				// Hits the bottom somewhere.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					raisedYBottom,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = -Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::NegativeY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Diagonal)
	{
		const VoxelData::DiagonalData &diagonal = voxelData.diagonal;
		const bool isRightDiag = diagonal.type1;

		// Generate points for the diagonal's quad.
		Double3 bottomLeftPoint, bottomRightPoint, topRightPoint;
		if (isRightDiag)
		{
			bottomLeftPoint = Double3(
				static_cast<double>(voxel.x),
				static_cast<double>(voxel.y) * ceilingHeight,
				static_cast<double>(voxel.z));
			bottomRightPoint = Double3(
				bottomLeftPoint.x + 1.0,
				bottomLeftPoint.y,
				bottomLeftPoint.z + 1.0);
			topRightPoint = Double3(
				bottomRightPoint.x,
				bottomRightPoint.y + ceilingHeight,
				bottomRightPoint.z);
		}
		else
		{
			bottomLeftPoint = Double3(
				static_cast<double>(voxel.x + 1),
				static_cast<double>(voxel.y) * ceilingHeight,
				static_cast<double>(voxel.z));
			bottomRightPoint = Double3(
				bottomLeftPoint.x - 1.0,
				bottomLeftPoint.y,
				bottomLeftPoint.z + 1.0);
			topRightPoint = Double3(
				bottomRightPoint.x,
				bottomRightPoint.y + ceilingHeight,
				bottomRightPoint.z);
		}

		Double3 hitPoint;
		const bool success = MathUtils::rayQuadIntersection(
			rayStart, rayDirection, bottomLeftPoint, bottomRightPoint, topRightPoint, &hitPoint);

		if (success)
		{
			const double t = (hitPoint - rayStart).length();
			hit.initVoxel(t, hitPoint, voxelID, voxel, nullptr);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Back faces of transparent walls are invisible.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		// See if the intersected facing and the edge's facing are the same, and only
		// consider edges with collision.
		const VoxelData::EdgeData &edge = voxelData.edge;
		const VoxelFacing edgeFacing = edge.facing;

		if ((edgeFacing == farFacing) && edge.collider)
		{
			// See if the ray hits within the edge with its Y offset.
			const double edgeYBottom = (static_cast<double>(voxel.y) + edge.yOffset) * ceilingHeight;
			const double edgeYTop = edgeYBottom + ceilingHeight;

			if ((farPoint.y >= edgeYBottom) && (farPoint.y <= edgeYTop))
			{
				const double t = (farPoint - rayStart).length();
				const Double3 hitPoint = farPoint;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Chasm)
	{
		// The chasm type determines the depth relative to the top of the voxel.
		const VoxelData::ChasmData &chasm = voxelData.chasm;
		const bool isDryChasm = chasm.type == VoxelData::ChasmData::Type::Dry;
		const double voxelHeight = isDryChasm ? ceilingHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

		const double chasmYTop = static_cast<double>(voxel.y + 1) * ceilingHeight;
		const double chasmYBottom = chasmYTop - voxelHeight;

		// See if the ray starts above or below the chasm floor.
		if (rayStart.y >= chasmYBottom)
		{
			// Above the floor. See which face the ray hits.
			if ((farFacing == VoxelFacing::NegativeY) || (farPoint.y < chasmYBottom))
			{
				// Hits the floor somewhere.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					chasmYBottom,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::NegativeY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else if (farFacing != VoxelFacing::PositiveY && chasm.faceIsVisible(farFacing))
			{
				// Hits a side wall.
				const double t = (farPoint - rayStart).length();
				const Double3 hitPoint = farPoint;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &farFacing);
				return true;
			}
			else
			{
				// Goes up outside the chasm, or goes through an invisible chasm side wall.
				return false;
			}
		}
		else
		{
			// Below the floor. See if the ray hits the bottom face.
			if (farPoint.y >= chasmYBottom)
			{
				// Hits the bottom face somewhere.
				const Double3 planeOrigin(
					static_cast<double>(voxel.x) + 0.50,
					chasmYBottom,
					static_cast<double>(voxel.z) + 0.50);
				const Double3 planeNormal = -Double3::UnitY;

				// Ray-plane intersection (guaranteed to hit a valid spot).
				Double3 hitPoint;
				const bool success = MathUtils::rayPlaneIntersection(
					rayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
				DebugAssert(success);

				const double t = (hitPoint - rayStart).length();
				const VoxelFacing facing = VoxelFacing::NegativeY;
				hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else if (voxelDataType == VoxelDataType::Door)
	{
		// Doors are not clickable when in the same voxel (besides, it would be complicated
		// due to various oddities: some doors have back faces, swinging doors have corner
		// preferences, etc.).
		return false;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelDataType)));
	}
}

bool Physics::testVoxelRay(const Double3 &rayStart, const Double3 &rayDirection,
	const Int3 &voxel, VoxelFacing nearFacing, const Double3 &nearPoint,
	const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid, Physics::Hit &hit)
{
	const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

	// Get the voxel data associated with the voxel.
	const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
	const VoxelDataType voxelDataType = voxelData.dataType;

	// @todo: decide later if all voxel types can just use one VoxelGeometry block of code
	// instead of branching on type here.

	// Determine which type the voxel data is and run the associated calculation.
	if (voxelDataType == VoxelDataType::None)
	{
		// Do nothing.
		return false;
	}
	else if (voxelDataType == VoxelDataType::Wall)
	{
		// Opaque walls are always hit.
		const double t = (nearPoint - rayStart).length();
		const Double3 hitPoint = nearPoint;
		hit.initVoxel(t, hitPoint, voxelID, voxel, &nearFacing);
		return true;
	}
	else if (voxelDataType == VoxelDataType::Floor)
	{
		// Intersect the floor as a quad.
		Quad quad;
		const int quadsWritten = VoxelGeometry::getQuads(voxelData, voxel, ceilingHeight, &quad, 1);
		DebugAssert(quadsWritten == 1);

		Double3 hitPoint;
		const bool success = MathUtils::rayQuadIntersection(
			rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

		if (success)
		{
			const double t = (hitPoint - rayStart).length();
			const VoxelFacing facing = VoxelFacing::PositiveY;
			hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Ceiling)
	{
		// Intersect the ceiling as a quad.
		Quad quad;
		const int quadsWritten = VoxelGeometry::getQuads(voxelData, voxel, ceilingHeight, &quad, 1);
		DebugAssert(quadsWritten == 1);

		Double3 hitPoint;
		const bool success = MathUtils::rayQuadIntersection(
			rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

		if (success)
		{
			const double t = (hitPoint - rayStart).length();
			const VoxelFacing facing = VoxelFacing::NegativeY;
			hit.initVoxel(t, hitPoint, voxelID, voxel, &facing);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Raised)
	{
		// Intersect each face of the platform and find the closest one (if any).
		int quadCount;
		VoxelGeometry::getInfo(voxelData, &quadCount);

		std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
		const int quadsWritten = VoxelGeometry::getQuads(
			voxelData, voxel, ceilingHeight, quads.data(), static_cast<int>(quads.size()));
		DebugAssert(quadsWritten == quadCount);

		double closestT = Hit::MAX_T;
		Double3 closestHitPoint;
		int closestIndex;

		for (int i = 0; i < quadsWritten; i++)
		{
			const Quad &quad = quads[i];

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - rayStart).length();
				if (t < closestT)
				{
					closestT = t;
					closestHitPoint = hitPoint;
					closestIndex = i;
				}
			}
		}

		if (closestT < Hit::MAX_T)
		{
			const Quad &closestQuad = quads[closestIndex];
			const Double3 normal = closestQuad.getNormal();
			VoxelFacing facing;
			if (TryGetFacingFromNormal(normal, &facing))
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, &facing);
			}
			else
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, nullptr);
			}

			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Diagonal)
	{
		// Intersect the diagonal as a quad.
		Quad quad;
		const int quadsWritten = VoxelGeometry::getQuads(voxelData, voxel, ceilingHeight, &quad, 1);
		DebugAssert(quadsWritten == 1);

		Double3 hitPoint;
		const bool success = MathUtils::rayQuadIntersection(
			rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

		if (success)
		{
			const double t = (hitPoint - rayStart).length();
			hit.initVoxel(t, hitPoint, voxelID, voxel, nullptr);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::TransparentWall)
	{
		// Intersect each face and find the closest one (if any).
		int quadCount;
		VoxelGeometry::getInfo(voxelData, &quadCount);

		std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
		const int quadsWritten = VoxelGeometry::getQuads(
			voxelData, voxel, ceilingHeight, quads.data(), static_cast<int>(quads.size()));
		DebugAssert(quadsWritten == quadCount);

		double closestT = Hit::MAX_T;
		Double3 closestHitPoint;
		int closestIndex;

		for (int i = 0; i < quadsWritten; i++)
		{
			const Quad &quad = quads[i];

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - rayStart).length();
				if (t < closestT)
				{
					closestT = t;
					closestHitPoint = hitPoint;
					closestIndex = i;
				}
			}
		}

		if (closestT < Hit::MAX_T)
		{
			const Quad &closestQuad = quads[closestIndex];
			const Double3 normal = closestQuad.getNormal();
			VoxelFacing facing;
			if (TryGetFacingFromNormal(normal, &facing))
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, &facing);
			}
			else
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, nullptr);
			}

			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Edge)
	{
		// Intersect the edge as a quad.
		Quad quad;
		const int quadsWritten = VoxelGeometry::getQuads(voxelData, voxel, ceilingHeight, &quad, 1);
		DebugAssert(quadsWritten == 1);

		Double3 hitPoint;
		const bool success = MathUtils::rayQuadIntersection(
			rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

		if (success)
		{
			const double t = (hitPoint - rayStart).length();
			hit.initVoxel(t, hitPoint, voxelID, voxel, nullptr);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Chasm)
	{
		// Intersect each face and find the closest one (if any).
		int quadCount;
		VoxelGeometry::getInfo(voxelData, &quadCount);

		std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
		const int quadsWritten = VoxelGeometry::getQuads(
			voxelData, voxel, ceilingHeight, quads.data(), static_cast<int>(quads.size()));
		DebugAssert(quadsWritten == quadCount);

		double closestT = Hit::MAX_T;
		Double3 closestHitPoint;
		int closestIndex;

		for (int i = 0; i < quadsWritten; i++)
		{
			const Quad &quad = quads[i];

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - rayStart).length();
				if (t < closestT)
				{
					closestT = t;
					closestHitPoint = hitPoint;
					closestIndex = i;
				}
			}
		}

		if (closestT < Hit::MAX_T)
		{
			const Quad &closestQuad = quads[closestIndex];
			const Double3 normal = closestQuad.getNormal();
			VoxelFacing facing;
			if (TryGetFacingFromNormal(normal, &facing))
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, &facing);
			}
			else
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, nullptr);
			}

			return true;
		}
		else
		{
			return false;
		}
	}
	else if (voxelDataType == VoxelDataType::Door)
	{
		// @todo: ideally this method would take any hit on a door into consideration, since
		// it's the calling code's responsibility to decide what to do based on the door's open
		// state, but for now it will assume closed doors only, for simplicity.

		// Intersect each face and find the closest one (if any).
		int quadCount;
		VoxelGeometry::getInfo(voxelData, &quadCount);

		std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
		const int quadsWritten = VoxelGeometry::getQuads(
			voxelData, voxel, ceilingHeight, quads.data(), static_cast<int>(quads.size()));
		DebugAssert(quadsWritten == quadCount);

		double closestT = Hit::MAX_T;
		Double3 closestHitPoint;
		int closestIndex;

		for (int i = 0; i < quadsWritten; i++)
		{
			const Quad &quad = quads[i];

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				rayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - rayStart).length();
				if (t < closestT)
				{
					closestT = t;
					closestHitPoint = hitPoint;
					closestIndex = i;
				}
			}
		}

		if (closestT < Hit::MAX_T)
		{
			const Quad &closestQuad = quads[closestIndex];
			const Double3 normal = closestQuad.getNormal();
			VoxelFacing facing;
			if (TryGetFacingFromNormal(normal, &facing))
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, &facing);
			}
			else
			{
				hit.initVoxel(closestT, closestHitPoint, voxelID, voxel, nullptr);
			}

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelDataType)));
	}
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &rayDirection, double ceilingHeight,
	const Double3 &cameraForward, bool pixelPerfect, const EntityManager &entityManager,
	const VoxelGrid &voxelGrid, const Renderer &renderer, Physics::Hit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.setT(Hit::MAX_T);

	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-cameraForward.x, 0.0, -cameraForward.z).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

	const Double2 rayStartXZ(rayStart.x, rayStart.z);

	const VoxelEntityMap voxelEntityMap = Physics::makeVoxelEntityMap(
		rayStart, rayDirection, ceilingHeight, voxelGrid, entityManager);

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
		rayDirection.x * rayDirection.x,
		rayDirection.y * rayDirection.y,
		rayDirection.z * rayDirection.z);

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
	const bool nonNegativeDirX = rayDirection.x >= 0.0;
	const bool nonNegativeDirY = rayDirection.y >= 0.0;
	const bool nonNegativeDirZ = rayDirection.z >= 0.0;

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

	constexpr double maxCellDistance = 10.0; // Arbitrary value
	while (voxelIsValid && (cellDistSquared < (maxCellDistance * maxCellDistance)))
	{
		// Get the index of the current voxel in the voxel grid.
		const int gridIndex = cell.x + (cell.y * gridWidth) + (cell.z * gridWidth * gridHeight);

#pragma region Ray Test this voxel
		// Check if the current voxel is solid.
		const uint16_t voxelID = voxels[gridIndex];
		const bool isAir = voxelID == 0;
		if (!isAir)
		{
			// Boolean for whether the ray ended in the same voxel it started in.
			const bool stoppedInFirstVoxel = cell == voxel;

			// Get the distance from the camera to the hit point. It is a special case
			// if the ray stopped in the first voxel.
			double distance;
			VoxelFacing facing = VoxelFacing::NegativeX;
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
						((1.0 - stepReal.x) / 2.0)) / rayDirection.x;
					facing = nonNegativeDirX ? VoxelFacing::NegativeX : VoxelFacing::PositiveX;
				}
				else if (axis == Axis::Y)
				{
					distance = ((static_cast<double>(cell.y) * voxelHeight) - rayStart.y +
						(((1.0 - stepReal.y) / 2.0) * voxelHeight)) / rayDirection.y;
					facing = VoxelFacing::NegativeZ; // TODO: There are no facing values for Y
				}
				else
				{
					distance = (static_cast<double>(cell.z) - rayStart.z +
						((1.0 - stepReal.z) / 2.0)) / rayDirection.z;
					facing = nonNegativeDirZ ? VoxelFacing::NegativeZ : VoxelFacing::PositiveZ;
				}
			}

			const Double3 rayEnd = rayStart + (rayDirection * distance);
			if (stoppedInFirstVoxel)
			{
				// @todo: make sure we're feeding the right facing. We want the one for this
				// voxel, not the next voxel (if that would even happen). Test by printing out
				// the facing when inside a wall voxel.
				testInitialVoxelRay(rayStart, rayDirection, cell, facing, rayEnd,
					ceilingHeight, voxelGrid, hit);
			}
			else
			{
				// @todo: ray points are definitely wrong -- rayStart and nearPoint should not
				// be the same variable. Fix this!
				testVoxelRay(rayStart, rayDirection, cell, facing, rayStart, rayEnd,
					ceilingHeight, voxelGrid, hit);
			}
		}

#pragma endregion Ray Test this voxel

#pragma region Ray Test any entities that cross this voxel

		// Check if there are any entites that cross the current voxel.
		const auto iter = voxelEntityMap.find(cell);
		if (iter != voxelEntityMap.end())
		{
			// Iterate over all the entities that cross this voxel and ray test them.
			const auto &entityDataList = iter->second;
			for (const auto &visData : entityDataList)
			{
				const Entity &entity = *visData.entity;
				const EntityData &entityData = *entityManager.getEntityData(entity.getDataIndex());

				const Double2 flatPosition2D(
					visData.flatPosition.x,
					visData.flatPosition.z);

				// Check if the flat is somewhere in front of the camera.
				const Double2 flatEyeDiff = flatPosition2D - rayStartXZ;
				const double flatEyeDiffLenSqr = flatEyeDiff.lengthSquared();

				if (flatEyeDiffLenSqr < hit.getTSqr())
				{
					const double flatWidth = visData.keyframe.getWidth();
					const double flatHeight = visData.keyframe.getHeight();
					const double flatHalfWidth = flatWidth * 0.50;

					Double3 hitPoint;
					if (renderer.getEntityRayIntersection(visData, entityData.getFlatIndex(),
						flatForward, flatRight, flatUp, flatWidth, flatHeight, rayStart,
						rayDirection, pixelPerfect, &hitPoint))
					{
						const double distance = (hitPoint - rayStart).length();
						if (distance < hit.getT())
						{
							hit.initEntity(distance, hitPoint, entity.getID());
						}
					}
				}
			}
		}

#pragma endregion Ray Test any entities that cross this voxel

		if (hit.getT() != Hit::MAX_T)
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
	return hit.getT() != Hit::MAX_T;
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &direction, const Double3 &cameraForward,
	bool pixelPerfect, const EntityManager &entityManager, const VoxelGrid &voxelGrid,
	const Renderer &renderer, Physics::Hit &hit)
{
	constexpr double ceilingHeight = 1.0;
	return Physics::rayCast(rayStart, direction, ceilingHeight, cameraForward, pixelPerfect,
		entityManager, voxelGrid, renderer, hit);
}
