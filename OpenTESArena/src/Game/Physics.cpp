#include <algorithm>
#include <cmath>

#include "Physics.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/MIFFile.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityType.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Quad.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/ChunkUtils.h"
#include "../World/LevelData.h"
#include "../World/VoxelFacing3D.h"
#include "../World/VoxelGeometry.h"
#include "../World/VoxelGrid.h"

#include "components/debug/Debug.h"

// @todo: allow hits on the insides of voxels until the renderer uses back-face culling (if ever).

namespace Physics
{
	using VoxelEntityMap = std::unordered_map<NewInt3, std::vector<EntityManager::EntityVisibilityData>>;

	// Converts the normal to the associated voxel facing on success. Not all conversions
	// exist, for example, diagonals have normals but do not have a voxel facing.
	bool TryGetFacingFromNormal(const Double3 &normal, VoxelFacing3D *outFacing)
	{
		DebugAssert(outFacing != nullptr);

		constexpr double oneMinusEpsilon = 1.0 - Constants::Epsilon;

		bool success = true;
		if (normal.dot(Double3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveX;
		}
		else if (normal.dot(-Double3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeX;
		}
		else if (normal.dot(Double3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveY;
		}
		else if (normal.dot(-Double3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeY;
		}
		else if (normal.dot(Double3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveZ;
		}
		else if (normal.dot(-Double3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeZ;
		}
		else
		{
			success = false;
		}

		return success;
	}

	// Builds a set of voxels that are at least partially touched by entities. Ignores entities
	// behind the camera.
	Physics::VoxelEntityMap makeVoxelEntityMap(const CoordDouble3 &cameraCoord, const NewDouble3 &cameraDirection,
		int chunkDistance, double ceilingHeight, const VoxelGrid &voxelGrid, const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary)
	{
		const NewDouble3 absoluteCameraPosition = VoxelUtils::coordToNewPoint(cameraCoord);
		const NewDouble2 absoluteCameraPositionXZ(absoluteCameraPosition.x, absoluteCameraPosition.z);
		const NewDouble2 cameraDirXZ(cameraDirection.x, cameraDirection.z);

		ChunkInt2 minChunk, maxChunk;
		ChunkUtils::getSurroundingChunks(cameraCoord.chunk, chunkDistance, &minChunk, &maxChunk);

		// Gather up entities in nearby chunks.
		const int totalNearbyEntities = [&entityManager, &minChunk, &maxChunk]()
		{
			int count = 0;
			for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
			{
				for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
				{
					count += entityManager.getTotalCountInChunk(ChunkInt2(x, z));
				}
			}

			return count;
		}();

		std::vector<const Entity*> entities(totalNearbyEntities, nullptr);
		int entityInsertIndex = 0;
		auto addEntitiesFromChunk = [&entityManager, &entities, &entityInsertIndex](SNInt chunkX, WEInt chunkZ)
		{
			const Entity **entitiesPtr = entities.data() + entityInsertIndex;
			const int size = static_cast<int>(entities.size()) - entityInsertIndex;
			const int writtenCount = entityManager.getTotalEntitiesInChunk(
				ChunkInt2(chunkX, chunkZ), entitiesPtr, size);
			DebugAssert(writtenCount <= size);
			entityInsertIndex += writtenCount;
		};

		for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
		{
			for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
			{
				addEntitiesFromChunk(x, z);
			}
		}

		// Build mappings of voxels to entities.
		VoxelEntityMap voxelEntityMap;
		for (const Entity *entityPtr : entities)
		{
			if (entityPtr == nullptr)
			{
				continue;
			}

			const Entity &entity = *entityPtr;

			// Skip any entities that are behind the camera.
			const NewDouble2 absoluteEntityPosition = VoxelUtils::coordToNewPoint(entity.getPosition());
			NewDouble2 entityPosEyeDiff = absoluteEntityPosition - absoluteCameraPositionXZ;
			if (cameraDirXZ.dot(entityPosEyeDiff) < 0.0)
			{
				continue;
			}

			const CoordDouble2 cameraCoordXZ = VoxelUtils::newPointToCoord(absoluteCameraPositionXZ);
			EntityManager::EntityVisibilityData visData;
			entityManager.getEntityVisibilityData(entity, cameraCoordXZ, ceilingHeight, voxelGrid,
				entityDefLibrary, visData);

			// Use a bounding box to determine which voxels the entity could be in.
			CoordDouble3 minPoint, maxPoint;
			entityManager.getEntityBoundingBox(entity, visData, entityDefLibrary, &minPoint, &maxPoint);

			const NewDouble3 absoluteMinPoint = VoxelUtils::coordToNewPoint(minPoint);
			const NewDouble3 absoluteMaxPoint = VoxelUtils::coordToNewPoint(maxPoint);

			// Only iterate over voxels the entity could be in (at least partially).
			// This loop should always hit at least 1 voxel.
			const SNInt startX = static_cast<SNInt>(std::floor(absoluteMinPoint.x));
			const SNInt endX = static_cast<SNInt>(std::floor(absoluteMaxPoint.x));
			const int startY = static_cast<int>(std::floor(absoluteMinPoint.y / ceilingHeight));
			const int endY = static_cast<int>(std::floor(absoluteMaxPoint.y / ceilingHeight));
			const WEInt startZ = static_cast<WEInt>(std::floor(absoluteMinPoint.z));
			const WEInt endZ = static_cast<WEInt>(std::floor(absoluteMaxPoint.z));

			for (WEInt z = startZ; z <= endZ; z++)
			{
				for (int y = startY; y <= endY; y++)
				{
					for (SNInt x = startX; x <= endX; x++)
					{
						const NewInt3 voxel(x, y, z);

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

	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	bool testInitialVoxelRay(const NewDouble3 &absoluteRayStart, const Double3 &rayDirection,
		const NewInt3 &voxel, VoxelFacing3D farFacing, const Double3 &farPoint, double ceilingHeight,
		const LevelData &levelData, Physics::Hit &hit)
	{
		const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
		const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

		// Get the voxel definition associated with the voxel.
		const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
		const ArenaTypes::VoxelType voxelType = voxelDef.type;

		// Determine which type the voxel data is and run the associated calculation.
		if (voxelType == ArenaTypes::VoxelType::None)
		{
			// Do nothing.
			return false;
		}
		else if (voxelType == ArenaTypes::VoxelType::Wall)
		{
			// Opaque walls are always hit.
			const double t = (farPoint - absoluteRayStart).length();
			const Double3 hitPoint = farPoint;
			const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
			hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
			return true;
		}
		else if (voxelType == ArenaTypes::VoxelType::Floor)
		{
			// Check if the ray hits the top of the voxel.
			if (farFacing == VoxelFacing3D::PositiveY)
			{
				const double t = (farPoint - absoluteRayStart).length();
				const Double3 hitPoint = farPoint;
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Ceiling)
		{
			// Check if the ray hits the bottom of the voxel.
			if (farFacing == VoxelFacing3D::NegativeY)
			{
				const double t = (farPoint - absoluteRayStart).length();
				const Double3 hitPoint = farPoint;
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Raised)
		{
			const VoxelDefinition::RaisedData &raised = voxelDef.raised;
			const double raisedYBottom = (static_cast<double>(voxel.y) + raised.yOffset) * ceilingHeight;
			const double raisedYTop = raisedYBottom + (raised.ySize * ceilingHeight);

			if ((absoluteRayStart.y > raisedYBottom) && (absoluteRayStart.y < raisedYTop))
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
					return true;
				}
				else
				{
					// Hits the inside wall of the raised platform.
					const double t = (farPoint - absoluteRayStart).length();
					const Double3 hitPoint = farPoint;
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
					return true;
				}
			}
			else if (absoluteRayStart.y > raisedYTop)
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (absoluteRayStart.y < raisedYBottom)
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
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
		else if (voxelType == ArenaTypes::VoxelType::Diagonal)
		{
			const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
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
				absoluteRayStart, rayDirection, bottomLeftPoint, bottomRightPoint, topRightPoint, &hitPoint);

			if (success)
			{
				const double t = (hitPoint - absoluteRayStart).length();
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				hit.initVoxel(t, hitPoint, voxelID, coord, nullptr);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::TransparentWall)
		{
			// Back faces of transparent walls are invisible.
			return false;
		}
		else if (voxelType == ArenaTypes::VoxelType::Edge)
		{
			// See if the intersected facing and the edge's facing are the same, and only
			// consider edges with collision.
			const VoxelDefinition::EdgeData &edge = voxelDef.edge;
			const VoxelFacing2D edgeFacing = edge.facing;
			const VoxelFacing3D edgeFacing3D = VoxelUtils::convertFaceTo3D(edgeFacing);

			if ((edgeFacing3D == farFacing) && edge.collider)
			{
				// See if the ray hits within the edge with its Y offset.
				const double edgeYBottom = (static_cast<double>(voxel.y) + edge.yOffset) * ceilingHeight;
				const double edgeYTop = edgeYBottom + ceilingHeight;

				if ((farPoint.y >= edgeYBottom) && (farPoint.y <= edgeYTop))
				{
					const double t = (farPoint - absoluteRayStart).length();
					const Double3 hitPoint = farPoint;
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
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
		else if (voxelType == ArenaTypes::VoxelType::Chasm)
		{
			// The chasm type determines the depth relative to the top of the voxel.
			const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
			const bool isDryChasm = chasm.type == ArenaTypes::ChasmType::Dry;
			const double voxelHeight = isDryChasm ? ceilingHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;

			const double chasmYTop = static_cast<double>(voxel.y + 1) * ceilingHeight;
			const double chasmYBottom = chasmYTop - voxelHeight;

			// See if the ray starts above or below the chasm floor.
			if (absoluteRayStart.y >= chasmYBottom)
			{
				// Get any non-default state for this chasm voxel.
				const VoxelInstance::ChasmState *chasmState = [&voxel, &levelData]() -> const VoxelInstance::ChasmState*
				{
					const VoxelInstance *voxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);
					if (voxelInst != nullptr)
					{
						return &voxelInst->getChasmState();
					}
					else
					{
						return nullptr;
					}
				}();

				// Above the floor. See which face the ray hits.
				if ((farFacing == VoxelFacing3D::NegativeY) || (farPoint.y < chasmYBottom))
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
					return true;
				}
				else if ((farFacing != VoxelFacing3D::PositiveY) &&
					((chasmState != nullptr) && chasmState->faceIsVisible(farFacing)))
				{
					// Hits a side wall.
					const double t = (farPoint - absoluteRayStart).length();
					const Double3 hitPoint = farPoint;
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					hit.initVoxel(t, hitPoint, voxelID, coord, &farFacing);
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
						absoluteRayStart, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);

					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Door)
		{
			// Doors are not clickable when in the same voxel (besides, it would be complicated
			// due to various oddities: some doors have back faces, swinging doors have corner
			// preferences, etc.).
			return false;
		}
		else
		{
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Checks a voxel for ray hits and writes them into the output parameter. Returns
	// true if the ray hit something.
	bool testVoxelRay(const NewDouble3 &absoluteRayStart, const Double3 &rayDirection, const NewInt3 &voxel,
		VoxelFacing3D nearFacing, const Double3 &nearPoint, const Double3 &farPoint,
		double ceilingHeight, const LevelData &levelData, Physics::Hit &hit)
	{
		const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
		const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);

		// Get the voxel definition associated with the voxel.
		const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
		const ArenaTypes::VoxelType voxelType = voxelDef.type;

		// @todo: decide later if all voxel types can just use one VoxelGeometry block of code
		// instead of branching on type here.

		// Determine which type the voxel data is and run the associated calculation.
		if (voxelType == ArenaTypes::VoxelType::None)
		{
			// Do nothing.
			return false;
		}
		else if (voxelType == ArenaTypes::VoxelType::Wall)
		{
			// Opaque walls are always hit.
			const double t = (nearPoint - absoluteRayStart).length();
			const Double3 hitPoint = nearPoint;
			const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
			hit.initVoxel(t, hitPoint, voxelID, coord, &nearFacing);
			return true;
		}
		else if (voxelType == ArenaTypes::VoxelType::Floor)
		{
			// Intersect the floor as a quad.
			Quad quad;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - absoluteRayStart).length();
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
				hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Ceiling)
		{
			// Intersect the ceiling as a quad.
			Quad quad;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - absoluteRayStart).length();
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
				hit.initVoxel(t, hitPoint, voxelID, coord, &facing);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Raised)
		{
			// Intersect each face of the platform and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, nullptr, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight,
				nullptr, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			Double3 closestHitPoint;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				Double3 hitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

				if (success)
				{
					const double t = (hitPoint - absoluteRayStart).length();
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
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const Double3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::TryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, nullptr);
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Diagonal)
		{
			// Intersect the diagonal as a quad.
			Quad quad;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			Double3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

			if (success)
			{
				const double t = (hitPoint - absoluteRayStart).length();
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				hit.initVoxel(t, hitPoint, voxelID, coord, nullptr);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::TransparentWall)
		{
			// Intersect each face and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, nullptr, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight,
				nullptr, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			Double3 closestHitPoint;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				Double3 hitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

				if (success)
				{
					const double t = (hitPoint - absoluteRayStart).length();
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
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const Double3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::TryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, nullptr);
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Edge)
		{
			const VoxelDefinition::EdgeData &edge = voxelDef.edge;

			if (edge.collider)
			{
				// Intersect the edge as a quad.
				Quad quad;
				const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight, nullptr, &quad, 1);
				DebugAssert(quadsWritten == 1);

				Double3 hitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

				if (success)
				{
					const double t = (hitPoint - absoluteRayStart).length();
					const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
					const VoxelFacing3D edgeFacing3D = VoxelUtils::convertFaceTo3D(edge.facing);
					hit.initVoxel(t, hitPoint, voxelID, coord, &edgeFacing3D);
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
		else if (voxelType == ArenaTypes::VoxelType::Chasm)
		{
			const VoxelInstance *voxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);

			// Intersect each face and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, voxelInst, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight,
				voxelInst, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			Double3 closestHitPoint;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				Double3 hitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

				if (success)
				{
					const double t = (hitPoint - absoluteRayStart).length();
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
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const Double3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::TryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, nullptr);
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		else if (voxelType == ArenaTypes::VoxelType::Door)
		{
			// @todo: ideally this method would take any hit on a door into consideration, since
			// it's the calling code's responsibility to decide what to do based on the door's open
			// state, but for now it will assume closed doors only, for simplicity.

			const VoxelInstance *voxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::OpenDoor);

			// Intersect each face and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, voxelInst, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, voxel, ceilingHeight,
				voxelInst, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			Double3 closestHitPoint;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				Double3 hitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayStart, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &hitPoint);

				if (success)
				{
					const double t = (hitPoint - absoluteRayStart).length();
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
				const CoordInt3 coord = VoxelUtils::newVoxelToCoord(voxel);
				const Double3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::TryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitPoint, voxelID, coord, nullptr);
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
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Helper function for testing which entities in a voxel are intersected by a ray.
	bool testEntitiesInVoxel(const NewDouble3 &absoluteRayStart, const Double3 &rayDirection,
		const Double3 &flatForward, const Double3 &flatRight, const Double3 &flatUp, const NewInt3 &voxel,
		const VoxelEntityMap &voxelEntityMap, bool pixelPerfect, const Palette &palette,
		const EntityManager &entityManager, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit)
	{
		// Use a separate hit variable so we can determine whether an entity was closer.
		Physics::Hit entityHit;
		entityHit.setT(Hit::MAX_T);

		const auto iter = voxelEntityMap.find(voxel);
		if (iter != voxelEntityMap.end())
		{
			// Iterate over all the entities that cross this voxel and ray test them.
			const auto &entityVisDataList = iter->second;
			for (const auto &visData : entityVisDataList)
			{
				const Entity &entity = *visData.entity;
				const EntityDefinition &entityDef = entityManager.getEntityDef(
					entity.getDefinitionID(), entityDefLibrary);
				const EntityAnimationDefinition::Keyframe &animKeyframe =
					entityManager.getEntityAnimKeyframe(entity, visData, entityDefLibrary);

				const double flatWidth = animKeyframe.getWidth();
				const double flatHeight = animKeyframe.getHeight();

				Double3 hitPoint;
				if (renderer.getEntityRayIntersection(visData, flatForward, flatRight, flatUp,
					flatWidth, flatHeight, absoluteRayStart, rayDirection, pixelPerfect, palette, &hitPoint))
				{
					const double distance = (hitPoint - absoluteRayStart).length();
					if (distance < entityHit.getT())
					{
						entityHit.initEntity(distance, hitPoint, entity.getID(), entity.getEntityType());
					}
				}
			}
		}

		const bool entityIsCloser = entityHit.getT() < hit.getT();
		if (entityIsCloser)
		{
			hit = entityHit;
			return true;
		}
		else
		{
			return false;
		}
	}

	// Internal ray casting loop for stepping through individual voxels and checking
	// ray intersections with voxel data and entities.
	template <bool NonNegativeDirX, bool NonNegativeDirY, bool NonNegativeDirZ>
	void rayCastInternal(const CoordDouble3 &rayStart, const NewDouble3 &rayDirection, const NewDouble3 &cameraForward,
		double ceilingHeight, const LevelData &levelData, const VoxelEntityMap &voxelEntityMap,
		bool pixelPerfect, const Palette &palette, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit)
	{
		const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
		const EntityManager &entityManager = levelData.getEntityManager();

		// Each flat shares the same axes. The forward direction always faces opposite to 
		// the camera direction.
		const Double3 flatForward = Double3(-cameraForward.x, 0.0, -cameraForward.z).normalized();
		const Double3 flatUp = Double3::UnitY;
		const Double3 flatRight = flatForward.cross(flatUp).normalized();

		// Axis length is the length of a voxel in each dimension. This is required for features
		// like tall voxels.
		const Double3 axisLen(1.0, ceilingHeight, 1.0);

		// Initial voxel as reals and integers.
		const NewDouble3 absoluteRayStart = VoxelUtils::coordToNewPoint(rayStart);
		const Double3 rayStartVoxelReal(
			std::floor(absoluteRayStart.x / axisLen.x),
			std::floor(absoluteRayStart.y / axisLen.y),
			std::floor(absoluteRayStart.z / axisLen.z));
		const NewInt3 rayStartVoxel(
			static_cast<int>(rayStartVoxelReal.x),
			static_cast<int>(rayStartVoxelReal.y),
			static_cast<int>(rayStartVoxelReal.z));

		// World space floor of the voxel the ray starts in, instead of grid space, adjusted for
		// voxel side lengths.
		const Double3 rayStartRelativeFloor = rayStartVoxelReal * axisLen;

		// Delta distance is how far the ray has to go to step one voxel's worth along a certain axis.
		// This is affected by non-uniform grid properties like tall voxels.
		const Double3 deltaDist(
			(NonNegativeDirX ? axisLen.x : -axisLen.x) / rayDirection.x,
			(NonNegativeDirY ? axisLen.y : -axisLen.y) / rayDirection.y,
			(NonNegativeDirZ ? axisLen.z : -axisLen.z) / rayDirection.z);

		DebugAssert(deltaDist.x >= 0.0);
		DebugAssert(deltaDist.y >= 0.0);
		DebugAssert(deltaDist.z >= 0.0);

		// Step is the voxel delta per step (always +/- 1). The initial delta distances are percentages
		// of the delta distances, dependent on the ray start position inside the voxel.
		constexpr int stepX = NonNegativeDirX ? 1 : -1;
		constexpr int stepY = NonNegativeDirY ? 1 : -1;
		constexpr int stepZ = NonNegativeDirZ ? 1 : -1;

		const double initialDeltaDistPercentX = NonNegativeDirX ?
			(1.0 - ((absoluteRayStart.x - rayStartRelativeFloor.x) / axisLen.x)) :
			((absoluteRayStart.x - rayStartRelativeFloor.x) / axisLen.x);
		const double initialDeltaDistPercentY = NonNegativeDirY ?
			(1.0 - ((absoluteRayStart.y - rayStartRelativeFloor.y) / axisLen.y)) :
			((absoluteRayStart.y - rayStartRelativeFloor.y) / axisLen.y);
		const double initialDeltaDistPercentZ = NonNegativeDirZ ?
			(1.0 - ((absoluteRayStart.z - rayStartRelativeFloor.z) / axisLen.z)) :
			((absoluteRayStart.z - rayStartRelativeFloor.z) / axisLen.z);

		DebugAssert(initialDeltaDistPercentX >= 0.0);
		DebugAssert(initialDeltaDistPercentX <= 1.0);
		DebugAssert(initialDeltaDistPercentY >= 0.0);
		DebugAssert(initialDeltaDistPercentY <= 1.0);
		DebugAssert(initialDeltaDistPercentZ >= 0.0);
		DebugAssert(initialDeltaDistPercentZ <= 1.0);

		// Initial delta distance is a fraction of delta distance based on the ray's position in
		// the initial voxel.
		const double initialDeltaDistX = deltaDist.x * initialDeltaDistPercentX;
		const double initialDeltaDistY = deltaDist.y * initialDeltaDistPercentY;
		const double initialDeltaDistZ = deltaDist.z * initialDeltaDistPercentZ;

		// The visible voxel facings for each axis depending on ray direction. The facing is opposite
		// to the direction (i.e. negative Y face if stepping upward).
		constexpr std::array<VoxelFacing3D, 3> visibleWallFacings =
		{
			NonNegativeDirX ? VoxelFacing3D::NegativeX : VoxelFacing3D::PositiveX,
			NonNegativeDirY ? VoxelFacing3D::NegativeY : VoxelFacing3D::PositiveY,
			NonNegativeDirZ ? VoxelFacing3D::NegativeZ : VoxelFacing3D::PositiveZ,
		};

		// The ray distance and intersected face of the voxel.
		double rayDistance;
		VoxelFacing3D facing;

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
			if ((initialDeltaDistX < initialDeltaDistY) && (initialDeltaDistX < initialDeltaDistZ))
			{
				rayDistance = initialDeltaDistX;
				facing = visibleWallFacings[0];
			}
			else if (initialDeltaDistY < initialDeltaDistZ)
			{
				rayDistance = initialDeltaDistY;
				facing = visibleWallFacings[1];
			}
			else
			{
				rayDistance = initialDeltaDistZ;
				facing = visibleWallFacings[2];
			}

			// The initial far point is the wall hit.
			const NewDouble3 initialFarPoint = absoluteRayStart + (rayDirection * rayDistance);

			// Test the initial voxel for ray intersections.
			bool success = Physics::testInitialVoxelRay(absoluteRayStart, rayDirection, rayStartVoxel,
				facing, initialFarPoint, ceilingHeight, levelData, hit);
			success |= Physics::testEntitiesInVoxel(absoluteRayStart, rayDirection, flatForward, flatRight,
				flatUp, rayStartVoxel, voxelEntityMap, pixelPerfect, palette, entityManager,
				entityDefLibrary, renderer, hit);

			if (success)
			{
				// The ray hit something in the initial voxel.
				return;
			}
		}

		// The current voxel coordinate in the DDA loop.
		NewInt3 currentVoxel(rayStartVoxel.x, rayStartVoxel.y, rayStartVoxel.z);

		// Delta distance sums in each component, starting at the initial wall hit. The lowest
		// component is the candidate for the next DDA loop.
		double deltaDistSumX = initialDeltaDistX;
		double deltaDistSumY = initialDeltaDistY;
		double deltaDistSumZ = initialDeltaDistZ;

		// Helper values for ray distance calculation.
		constexpr double halfOneMinusStepXReal = static_cast<double>((1 - stepX) / 2);
		constexpr double halfOneMinusStepYReal = static_cast<double>((1 - stepY) / 2);
		constexpr double halfOneMinusStepZReal = static_cast<double>((1 - stepZ) / 2);

		// Lambda for stepping to the next voxel coordinate in the grid and updating the ray distance.
		auto doDDAStep = [&absoluteRayStart, &rayDirection, &voxelGrid, &deltaDist, stepX, stepY, stepZ,
			initialDeltaDistX, initialDeltaDistY, initialDeltaDistZ, &visibleWallFacings,
			&rayDistance, &facing, &voxelIsValid, &currentVoxel, &deltaDistSumX, &deltaDistSumY,
			&deltaDistSumZ, halfOneMinusStepXReal, halfOneMinusStepYReal, halfOneMinusStepZReal]()
		{
			if ((deltaDistSumX < deltaDistSumY) && (deltaDistSumX < deltaDistSumZ))
			{
				deltaDistSumX += deltaDist.x;
				currentVoxel.x += stepX;
				facing = visibleWallFacings[0];
				voxelIsValid &= (currentVoxel.x >= 0) && (currentVoxel.x < voxelGrid.getWidth());
				rayDistance = ((static_cast<double>(currentVoxel.x) - absoluteRayStart.x) +
					halfOneMinusStepXReal) / rayDirection.x;
			}
			else if (deltaDistSumY < deltaDistSumZ)
			{
				deltaDistSumY += deltaDist.y;
				currentVoxel.y += stepY;
				facing = visibleWallFacings[1];
				voxelIsValid &= (currentVoxel.y >= 0) && (currentVoxel.y < voxelGrid.getHeight());
				rayDistance = ((static_cast<double>(currentVoxel.y) - absoluteRayStart.y) +
					halfOneMinusStepYReal) / rayDirection.y;
			}
			else
			{
				deltaDistSumZ += deltaDist.z;
				currentVoxel.z += stepZ;
				facing = visibleWallFacings[2];
				voxelIsValid &= (currentVoxel.z >= 0) && (currentVoxel.z < voxelGrid.getDepth());
				rayDistance = ((static_cast<double>(currentVoxel.z) - absoluteRayStart.z) +
					halfOneMinusStepZReal) / rayDirection.z;
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
			const NewInt3 savedVoxel = currentVoxel;
			const VoxelFacing3D savedFacing = facing;
			const double savedDistance = rayDistance;

			// Decide which voxel to step to next, and update the ray distance.
			doDDAStep();

			// Near and far points in the voxel. The near point is where the wall was hit before, and 
			// the far point is where the wall was just hit on the far side.
			const NewDouble3 nearPoint = absoluteRayStart + (rayDirection * savedDistance);
			const NewDouble3 farPoint = absoluteRayStart + (rayDirection * rayDistance);

			// Test the current voxel for ray intersections.
			bool success = Physics::testVoxelRay(absoluteRayStart, rayDirection, savedVoxel, savedFacing,
				nearPoint, farPoint, axisLen.y, levelData, hit);
			success |= Physics::testEntitiesInVoxel(absoluteRayStart, rayDirection, flatForward, flatRight,
				flatUp, savedVoxel, voxelEntityMap, pixelPerfect, palette, entityManager, entityDefLibrary,
				renderer, hit);

			if (success)
			{
				// The ray hit something in a voxel.
				break;
			}
		}
	}
}

void Physics::Hit::initVoxel(double t, const Double3 &point, uint16_t id, const CoordInt3 &coord,
	const VoxelFacing3D *facing)
{
	this->t = t;
	this->point = point;
	this->type = Hit::Type::Voxel;
	this->voxelHit.id = id;
	this->voxelHit.coord = coord;

	if (facing != nullptr)
	{
		this->voxelHit.facing = *facing;
	}
	else
	{
		this->voxelHit.facing = std::nullopt;
	}
}

void Physics::Hit::initEntity(double t, const Double3 &point, EntityID id, EntityType type)
{
	this->t = t;
	this->point = point;
	this->type = Hit::Type::Entity;
	this->entityHit.id = id;
	this->entityHit.type = type;
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

bool Physics::rayCast(const CoordDouble3 &rayStart, const NewDouble3 &rayDirection, int chunkDistance,
	double ceilingHeight, const NewDouble3 &cameraForward, bool pixelPerfect, const Palette &palette,
	bool includeEntities, const LevelData &levelData, const EntityDefinitionLibrary &entityDefLibrary,
	const Renderer &renderer, Physics::Hit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.setT(Hit::MAX_T);

	VoxelEntityMap voxelEntityMap;
	if (includeEntities)
	{
		const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
		const EntityManager &entityManager = levelData.getEntityManager();
		voxelEntityMap = Physics::makeVoxelEntityMap(rayStart, rayDirection, chunkDistance,
			ceilingHeight, voxelGrid, entityManager, entityDefLibrary);
	}

	// Ray cast through the voxel grid, populating the output hit data. Use the ray direction
	// booleans for better code generation (at the expense of having a pile of if/else branches
	// here).
	const bool nonNegativeDirX = rayDirection.x >= 0.0;
	const bool nonNegativeDirY = rayDirection.y >= 0.0;
	const bool nonNegativeDirZ = rayDirection.z >= 0.0;

	if (nonNegativeDirX)
	{
		if (nonNegativeDirY)
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, true, true>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
			else
			{
				Physics::rayCastInternal<true, true, false>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, false, true>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
			else
			{
				Physics::rayCastInternal<true, false, false>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
		}
	}
	else
	{
		if (nonNegativeDirY)
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, true, true>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
			else
			{
				Physics::rayCastInternal<false, true, false>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, false, true>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
			else
			{
				Physics::rayCastInternal<false, false, false>(rayStart, rayDirection, cameraForward, ceilingHeight,
					levelData, voxelEntityMap, pixelPerfect, palette, entityDefLibrary, renderer, hit);
			}
		}
	}

	// Return whether the ray hit something.
	return hit.getT() < Hit::MAX_T;
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const NewDouble3 &rayDirection, int chunkDistance,
	const NewDouble3 &cameraForward, bool pixelPerfect, const Palette &palette, bool includeEntities,
	const LevelData &levelData, const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer,
	Physics::Hit &hit)
{
	constexpr double ceilingHeight = 1.0;
	return Physics::rayCast(rayStart, rayDirection, chunkDistance, ceilingHeight, cameraForward, pixelPerfect,
		palette, includeEntities, levelData, entityDefLibrary, renderer, hit);
}
