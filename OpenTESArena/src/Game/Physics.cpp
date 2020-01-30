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

		// Skip any entities that are behind the camera.
		Double2 entityPosEyeDiff = entity.getPosition() - cameraPosXZ;
		if (cameraDirXZ.dot(entityPosEyeDiff) < 0.0)
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
		const VoxelData::EdgeData &edge = voxelData.edge;

		if (edge.collider)
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

bool Physics::testEntitiesInVoxel(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &flatForward, const Double3 &flatRight, const Double3 &flatUp,
	const Int3 &voxel, const VoxelEntityMap &voxelEntityMap, bool pixelPerfect,
	const EntityManager &entityManager, const Renderer &renderer, Physics::Hit &hit)
{
	// Save the current hit distance so we can determine whether an entity was closer.
	const double savedT = hit.getT();

	const Double2 rayStartXZ(rayStart.x, rayStart.z);

	const auto iter = voxelEntityMap.find(voxel);
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

	// Return whether an entity hit was closer than a voxel hit.
	return hit.getT() < savedT;
}

void Physics::rayCastInternal(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &cameraForward, double ceilingHeight, const VoxelGrid &voxelGrid,
	const VoxelEntityMap &voxelEntityMap, bool pixelPerfect, const EntityManager &entityManager,
	const Renderer &renderer, Physics::Hit &hit)
{
	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-cameraForward.x, 0.0, -cameraForward.z).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

	// Axis length is the length of a voxel in each dimension. This is required for features
	// like tall voxels.
	const Double3 axisLen(1.0, ceilingHeight, 1.0);

	// Initial voxel as reals and integers.
	const Double3 rayStartVoxelReal(
		std::floor(rayStart.x / axisLen.x),
		std::floor(rayStart.y / axisLen.y),
		std::floor(rayStart.z / axisLen.z));
	const Int3 rayStartVoxel(
		static_cast<int>(rayStartVoxelReal.x),
		static_cast<int>(rayStartVoxelReal.y),
		static_cast<int>(rayStartVoxelReal.z));

	// World space floor of the voxel the ray starts in, instead of grid space, adjusted for
	// voxel side lengths.
	const Double3 rayStartRelativeFloor = rayStartVoxelReal * axisLen;

	const bool nonNegativeDirX = rayDirection.x >= 0.0;
	const bool nonNegativeDirY = rayDirection.y >= 0.0;
	const bool nonNegativeDirZ = rayDirection.z >= 0.0;

	// Delta distance is how far the ray has to go to step one voxel's worth along a certain axis.
	// This is affected by non-uniform grid properties like tall voxels.
	const Double3 deltaDist(
		(nonNegativeDirX ? axisLen.x : -axisLen.x) / rayDirection.x,
		(nonNegativeDirY ? axisLen.y : -axisLen.y) / rayDirection.y,
		(nonNegativeDirZ ? axisLen.z : -axisLen.z) / rayDirection.z);

	DebugAssert(deltaDist.x >= 0.0);
	DebugAssert(deltaDist.y >= 0.0);
	DebugAssert(deltaDist.z >= 0.0);

	// Step is the voxel delta per step (always +/- 1). The initial delta distances are percentages
	// of the delta distances, dependent on the ray start position inside the voxel.
	Int3 step;
	Double3 initialDeltaDistPercents;
	if (nonNegativeDirX)
	{
		step.x = 1;
		initialDeltaDistPercents.x = 1.0 - ((rayStart.x - rayStartRelativeFloor.x) / axisLen.x);
	}
	else
	{
		step.x = -1;
		initialDeltaDistPercents.x = (rayStart.x - rayStartRelativeFloor.x) / axisLen.x;
	}

	if (nonNegativeDirY)
	{
		step.y = 1;
		initialDeltaDistPercents.y = 1.0 - ((rayStart.y - rayStartRelativeFloor.y) / axisLen.y);
	}
	else
	{
		step.y = -1;
		initialDeltaDistPercents.y = (rayStart.y - rayStartRelativeFloor.y) / axisLen.y;
	}

	if (nonNegativeDirZ)
	{
		step.z = 1;
		initialDeltaDistPercents.z = 1.0 - ((rayStart.z - rayStartRelativeFloor.z) / axisLen.z);
	}
	else
	{
		step.z = -1;
		initialDeltaDistPercents.z = (rayStart.z - rayStartRelativeFloor.z) / axisLen.z;
	}

	DebugAssert(initialDeltaDistPercents.x >= 0.0);
	DebugAssert(initialDeltaDistPercents.x <= 1.0);
	DebugAssert(initialDeltaDistPercents.y >= 0.0);
	DebugAssert(initialDeltaDistPercents.y <= 1.0);
	DebugAssert(initialDeltaDistPercents.z >= 0.0);
	DebugAssert(initialDeltaDistPercents.z <= 1.0);

	// Initial delta distance is a fraction of delta distance based on the ray's position in
	// the initial voxel.
	Double3 initialDeltaDist = deltaDist * initialDeltaDistPercents;

	// The visible voxel facings for each axis depending on ray direction. The facing is opposite
	// to the direction (i.e. negative Y face if stepping upward).
	const std::array<VoxelFacing, 3> visibleWallFacings =
	{
		nonNegativeDirX ? VoxelFacing::NegativeX : VoxelFacing::PositiveX,
		nonNegativeDirY ? VoxelFacing::NegativeY : VoxelFacing::PositiveY,
		nonNegativeDirZ ? VoxelFacing::NegativeZ : VoxelFacing::PositiveZ,
	};

	// The ray distance and intersected face of the voxel.
	double rayDistance;
	VoxelFacing facing;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid = (rayStartVoxel.x >= 0) && (rayStartVoxel.y >= 0) &&
		(rayStartVoxel.z >= 0) && (rayStartVoxel.x < voxelGrid.getWidth()) &&
		(rayStartVoxel.y < voxelGrid.getHeight()) && (rayStartVoxel.z < voxelGrid.getDepth());

	// The initial DDA step is a special case, so it's brought outside the DDA loop. This
	// complicates things a little bit, but it's important enough that it should be kept.
	if (voxelIsValid)
	{
		// See how far away the initial wall is, and which voxel face was hit. This is basically
		// "find min element index in array".
		if ((initialDeltaDist.x < initialDeltaDist.y) && (initialDeltaDist.x < initialDeltaDist.z))
		{
			rayDistance = initialDeltaDist.x;
			facing = visibleWallFacings[0];
		}
		else if (initialDeltaDist.y < initialDeltaDist.z)
		{
			rayDistance = initialDeltaDist.y;
			facing = visibleWallFacings[1];
		}
		else
		{
			rayDistance = initialDeltaDist.z;
			facing = visibleWallFacings[2];
		}

		// The initial far point is the wall hit.
		const Double3 initialFarPoint = rayStart + (rayDirection * rayDistance);

		// Test the initial voxel for ray intersections.
		bool success = Physics::testInitialVoxelRay(rayStart, rayDirection, rayStartVoxel,
			facing, initialFarPoint, ceilingHeight, voxelGrid, hit);
		success |= Physics::testEntitiesInVoxel(rayStart, rayDirection, flatForward, flatRight,
			flatUp, rayStartVoxel, voxelEntityMap, pixelPerfect, entityManager, renderer, hit);

		if (success)
		{
			// The ray hit something in the initial voxel.
			return;
		}
	}

	// The current voxel coordinate in the DDA loop.
	Int3 currentVoxel(rayStartVoxel.x, rayStartVoxel.y, rayStartVoxel.z);

	// Delta distance sums in each component, starting at the initial wall hit. The lowest
	// component is the candidate for the next DDA loop.
	Double3 deltaDistSum = initialDeltaDist;

	// Lambda for stepping to the next voxel coordinate in the grid and updating the ray distance.
	auto doDDAStep = [&rayStart, &rayDirection, &voxelGrid, &deltaDist, nonNegativeDirX,
		nonNegativeDirY, nonNegativeDirZ, &step, &initialDeltaDist, &visibleWallFacings,
		&rayDistance, &facing, &voxelIsValid, &currentVoxel, &deltaDistSum]()
	{
		if ((deltaDistSum.x < deltaDistSum.y) && (deltaDistSum.x < deltaDistSum.z))
		{
			deltaDistSum.x += deltaDist.x;
			currentVoxel.x += step.x;
			facing = visibleWallFacings[0];
			voxelIsValid &= (currentVoxel.x >= 0) && (currentVoxel.x < voxelGrid.getWidth());

			// @todo: probably needs to include axisLen for each X/Y/Z case? MUST VISUALIZE THIS!
			rayDistance = (static_cast<double>(currentVoxel.x) -
				rayStart.x + static_cast<double>((1 - step.x) / 2)) / rayDirection.x;
		}
		else if (deltaDistSum.y < deltaDistSum.z)
		{
			deltaDistSum.y += deltaDist.y;
			currentVoxel.y += step.y;
			facing = visibleWallFacings[1];
			voxelIsValid &= (currentVoxel.y >= 0) && (currentVoxel.y < voxelGrid.getHeight());
			rayDistance = (static_cast<double>(currentVoxel.y) -
				rayStart.y + static_cast<double>((1 - step.y) / 2)) / rayDirection.y;
		}
		else
		{
			deltaDistSum.z += deltaDist.z;
			currentVoxel.z += step.z;
			facing = visibleWallFacings[2];
			voxelIsValid &= (currentVoxel.z >= 0) && (currentVoxel.z < voxelGrid.getDepth());
			rayDistance = (static_cast<double>(currentVoxel.z) -
				rayStart.z + static_cast<double>((1 - step.z) / 2)) / rayDirection.z;
		}
	};

	// Step forward in the grid once to leave the initial voxel and update the ray distance.
	doDDAStep();

	// Step through the grid while the current voxel coordinate is valid. There doesn't
	// really need to be a max distance check here.
	while (voxelIsValid)
	{
		// Store part of the current DDA state. The loop needs to do another DDA step to calculate
		// the point on the far side of this voxel.
		const Int3 savedVoxel = currentVoxel;
		const VoxelFacing savedFacing = facing;
		const double savedDistance = rayDistance;

		// Decide which voxel to step to next, and update the ray distance.
		doDDAStep();

		// Near and far points in the voxel. The near point is where the wall was hit before, and 
		// the far point is where the wall was just hit on the far side.
		const Double3 nearPoint = rayStart + (rayDirection * savedDistance);
		const Double3 farPoint = rayStart + (rayDirection * rayDistance);

		// Test the current voxel for ray intersections.
		bool success = Physics::testVoxelRay(rayStart, rayDirection, savedVoxel, savedFacing,
			nearPoint, farPoint, axisLen.y, voxelGrid, hit);
		success |= Physics::testEntitiesInVoxel(rayStart, rayDirection, flatForward, flatRight,
			flatUp, savedVoxel, voxelEntityMap, pixelPerfect, entityManager, renderer, hit);

		if (success)
		{
			// The ray hit something in a voxel.
			break;
		}
	}
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &rayDirection, double ceilingHeight,
	const Double3 &cameraForward, bool pixelPerfect, bool includeEntities,
	const EntityManager &entityManager, const VoxelGrid &voxelGrid, const Renderer &renderer,
	Physics::Hit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.setT(Hit::MAX_T);

	VoxelEntityMap voxelEntityMap;
	if (includeEntities)
	{
		voxelEntityMap = Physics::makeVoxelEntityMap(
			rayStart, rayDirection, ceilingHeight, voxelGrid, entityManager);
	}

	// Ray cast through the voxel grid, populating the output hit data.
	Physics::rayCastInternal(rayStart, rayDirection, cameraForward, ceilingHeight, voxelGrid,
		voxelEntityMap, pixelPerfect, entityManager, renderer, hit);

	// Return whether the ray hit something.
	return hit.getT() < Hit::MAX_T;
}

bool Physics::rayCast(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &cameraForward, bool pixelPerfect, bool includeEntities,
	const EntityManager &entityManager, const VoxelGrid &voxelGrid, const Renderer &renderer,
	Physics::Hit &hit)
{
	constexpr double ceilingHeight = 1.0;
	return Physics::rayCast(rayStart, rayDirection, ceilingHeight, cameraForward, pixelPerfect,
		includeEntities, entityManager, voxelGrid, renderer, hit);
}
