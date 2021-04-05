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
#include "../World/LevelInstance.h"
#include "../World/LevelUtils.h"
#include "../World/VoxelFacing3D.h"
#include "../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

// @todo: allow hits on the insides of voxels until the renderer uses back-face culling (if ever).

namespace Physics
{
	// Container of the voxels each entity is touching per chunk. Each chunk needs to look at adjacent chunk
	// entities in case some of them overlap the chunk edge.
	struct ChunkEntityMap
	{
		ChunkInt2 chunk;
		std::unordered_map<VoxelInt3, std::vector<EntityManager::EntityVisibilityState3D>> mappings;

		void init(const ChunkInt2 &chunk)
		{
			this->chunk = chunk;
		}

		void add(const VoxelInt3 &voxel, const EntityManager::EntityVisibilityState3D &visState)
		{
			auto iter = this->mappings.find(voxel);
			if (iter == this->mappings.end())
			{
				iter = this->mappings.emplace(voxel, std::vector<EntityManager::EntityVisibilityState3D>()).first;
			}

			std::vector<EntityManager::EntityVisibilityState3D> &visStateList = iter->second;
			visStateList.emplace_back(visState);
		}
	};

	// Converts the normal to the associated voxel facing on success. Not all conversions
	// exist, for example, diagonals have normals but do not have a voxel facing.
	bool tryGetFacingFromNormal(const NewDouble3 &normal, VoxelFacing3D *outFacing)
	{
		DebugAssert(outFacing != nullptr);

		constexpr double oneMinusEpsilon = 1.0 - Constants::Epsilon;

		bool success = true;
		if (normal.dot(NewDouble3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveX;
		}
		else if (normal.dot(-NewDouble3::UnitX) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeX;
		}
		else if (normal.dot(NewDouble3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveY;
		}
		else if (normal.dot(-NewDouble3::UnitY) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeY;
		}
		else if (normal.dot(NewDouble3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::PositiveZ;
		}
		else if (normal.dot(-NewDouble3::UnitZ) > oneMinusEpsilon)
		{
			*outFacing = VoxelFacing3D::NegativeZ;
		}
		else
		{
			success = false;
		}

		return success;
	}

	// Builds a set of voxels for a chunk that are at least partially touched by entities. A point of reference
	// is needed for evaluating entity animations. Ignores entities behind the camera.
	// @todo: the view coord might eventually be obsolete if we use the view-independent bounding boxes of
	// entities instead. It would require a preprocessing step on the entity animation def, and would require
	// iterating all states and frames.
	Physics::ChunkEntityMap makeChunkEntityMap(const ChunkInt2 &chunk, const CoordDouble3 &viewCoord,
		double ceilingScale, const ChunkManager &chunkManager, const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary)
	{
		// Include entities within one chunk of the center chunk to get entities that are partially touching
		// the center chunk.
		constexpr int chunkDistance = 1;
		ChunkInt2 minChunk, maxChunk;
		ChunkUtils::getSurroundingChunks(chunk, chunkDistance, &minChunk, &maxChunk);

		// Gather up entities in nearby chunks.
		const int totalNearbyEntities = [&entityManager, &minChunk, &maxChunk]()
		{
			int count = 0;
			for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
			{
				for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
				{
					count += entityManager.getCountInChunk(ChunkInt2(x, z));
				}
			}

			return count;
		}();

		Buffer<const Entity*> entities(totalNearbyEntities);
		entities.fill(nullptr);

		int entityInsertIndex = 0;
		auto addEntitiesFromChunk = [&entityManager, &entities, &entityInsertIndex](SNInt chunkX, WEInt chunkZ)
		{
			const Entity **entitiesPtr = entities.get() + entityInsertIndex;
			const int size = entities.getCount() - entityInsertIndex;
			const int writtenCount = entityManager.getEntitiesInChunk(ChunkInt2(chunkX, chunkZ), entitiesPtr, size);
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

		ChunkEntityMap chunkEntityMap;
		chunkEntityMap.init(chunk);

		// Build mappings of voxels to entities.
		for (int i = 0; i < entities.getCount(); i++)
		{
			const Entity *entityPtr = entities.get(i);
			if (entityPtr == nullptr)
			{
				continue;
			}

			const Entity &entity = *entityPtr;
			const CoordDouble2 viewCoordXZ(viewCoord.chunk, VoxelDouble2(viewCoord.point.x, viewCoord.point.z));
			EntityManager::EntityVisibilityState3D visState;
			entityManager.getEntityVisibilityState3D(entity, viewCoordXZ, ceilingScale, chunkManager,
				entityDefLibrary, visState);

			// Get the entity's view-dependent bounding box to help determine which voxels they are in.
			// @todo: the view-independent bounding box would be better and would keep this function from needing a view coord.
			CoordDouble3 minCoord, maxCoord;
			entityManager.getEntityBoundingBox(entity, visState, entityDefLibrary, &minCoord, &maxCoord);

			// Normalize Y values.
			const VoxelDouble3 minPoint(minCoord.point.x, minCoord.point.y / ceilingScale, minCoord.point.z);
			const VoxelDouble3 maxPoint(maxCoord.point.x, maxCoord.point.y / ceilingScale, maxCoord.point.z);

			// Get min and max coordinates in chunk space and get the difference for iteration.
			const CoordInt3 minVoxelCoord(minCoord.chunk, VoxelUtils::pointToVoxel(minPoint));
			const CoordInt3 maxVoxelCoord(maxCoord.chunk, VoxelUtils::pointToVoxel(maxPoint));
			const VoxelInt3 voxelCoordDiff = maxVoxelCoord - minVoxelCoord;

			// Iterate over the voxels the entity's bounding box touches.
			for (WEInt z = 0; z <= voxelCoordDiff.z; z++)
			{
				for (int y = 0; y <= voxelCoordDiff.y; y++)
				{
					for (SNInt x = 0; x <= voxelCoordDiff.x; x++)
					{
						const VoxelInt3 curVoxel(
							minVoxelCoord.voxel.x + x,
							minVoxelCoord.voxel.y + y,
							minVoxelCoord.voxel.z + z);
						const CoordInt3 curCoord = ChunkUtils::recalculateCoord(minVoxelCoord.chunk, curVoxel);

						// If it's in the center chunk, add a mapping.
						if (curCoord.chunk == chunk)
						{
							chunkEntityMap.add(curCoord.voxel, visState);
						}
					}
				}
			}
		}

		return chunkEntityMap;
	}

	// The given chunk coordinate is known to be loaded.
	const ChunkEntityMap &getOrAddChunkEntityMap(const ChunkInt2 &chunk, const CoordDouble3 &viewCoord,
		double ceilingScale, const ChunkManager &chunkManager, const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary, std::vector<ChunkEntityMap> &chunkEntityMaps)
	{
		for (const ChunkEntityMap &map : chunkEntityMaps)
		{
			if (map.chunk == chunk)
			{
				return map;
			}
		}

		ChunkEntityMap newMap = Physics::makeChunkEntityMap(chunk, viewCoord, ceilingScale, chunkManager,
			entityManager, entityDefLibrary);
		chunkEntityMaps.emplace_back(std::move(newMap));
		return chunkEntityMaps.back();
	}

	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	bool testInitialVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const VoxelInt3 &voxel,
		VoxelFacing3D farFacing, const VoxelDouble3 &farPoint, double ceilingScale, const LevelInstance &levelInst,
		Physics::Hit &hit)
	{
		const ChunkManager &chunkManager = levelInst.getChunkManager();
		const Chunk *chunk = chunkManager.tryGetChunk(rayCoord.chunk);
		if (chunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		if (!chunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		const Chunk::VoxelID voxelID = chunk->getVoxel(voxel.x, voxel.y, voxel.z);

		// Get the voxel definition associated with the voxel.
		const VoxelDefinition &voxelDef = chunk->getVoxelDef(voxelID);
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
			const double t = (farPoint - rayCoord.point).length();
			const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
			hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
			return true;
		}
		else if (voxelType == ArenaTypes::VoxelType::Floor)
		{
			// Check if the ray hits the top of the voxel.
			if (farFacing == VoxelFacing3D::PositiveY)
			{
				const double t = (farPoint - rayCoord.point).length();
				const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
				hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
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
				const double t = (farPoint - rayCoord.point).length();
				const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
				hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
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
			const double raisedYBottom = (static_cast<double>(voxel.y) + raised.yOffset) * ceilingScale;
			const double raisedYTop = raisedYBottom + (raised.ySize * ceilingScale);

			if ((rayCoord.point.y > raisedYBottom) && (rayCoord.point.y < raisedYTop))
			{
				// Inside the raised platform. See where the far point is.
				if (farPoint.y < raisedYBottom)
				{
					// Hits the inside floor of the raised platform.
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						raisedYBottom,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
					return true;
				}
				else if (farPoint.y > raisedYTop)
				{
					// Hits the inside ceiling of the raised platform.
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						raisedYTop,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = -Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
					return true;
				}
				else
				{
					// Hits the inside wall of the raised platform.
					const double t = (farPoint - rayCoord.point).length();
					const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
					hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
					return true;
				}
			}
			else if (rayCoord.point.y > raisedYTop)
			{
				// Above the raised platform. See if the ray hits the top.
				if (farPoint.y <= raisedYTop)
				{
					// Hits the top somewhere.
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						raisedYTop,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (rayCoord.point.y < raisedYBottom)
			{
				// Below the raised platform. See if the ray hits the bottom.
				if (farPoint.y >= raisedYBottom)
				{
					// Hits the bottom somewhere.
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						raisedYBottom,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = -Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
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
			VoxelDouble3 bottomLeftPoint, bottomRightPoint, topRightPoint;
			if (isRightDiag)
			{
				bottomLeftPoint = VoxelDouble3(
					static_cast<SNDouble>(voxel.x),
					static_cast<double>(voxel.y) * ceilingScale,
					static_cast<WEDouble>(voxel.z));
				bottomRightPoint = VoxelDouble3(
					bottomLeftPoint.x + 1.0,
					bottomLeftPoint.y,
					bottomLeftPoint.z + 1.0);
				topRightPoint = VoxelDouble3(
					bottomRightPoint.x,
					bottomRightPoint.y + ceilingScale,
					bottomRightPoint.z);
			}
			else
			{
				bottomLeftPoint = VoxelDouble3(
					static_cast<SNDouble>(voxel.x + 1),
					static_cast<double>(voxel.y) * ceilingScale,
					static_cast<WEDouble>(voxel.z));
				bottomRightPoint = VoxelDouble3(
					bottomLeftPoint.x - 1.0,
					bottomLeftPoint.y,
					bottomLeftPoint.z + 1.0);
				topRightPoint = VoxelDouble3(
					bottomRightPoint.x,
					bottomRightPoint.y + ceilingScale,
					bottomRightPoint.z);
			}

			VoxelDouble3 hitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				rayCoord.point, rayDirection, bottomLeftPoint, bottomRightPoint, topRightPoint, &hitPoint);
			CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

			if (success)
			{
				const double t = (hitPoint - rayCoord.point).length();
				hit.initVoxel(t, hitCoord, voxelID, voxel, nullptr);
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
				const double edgeYBottom = (static_cast<double>(voxel.y) + edge.yOffset) * ceilingScale;
				const double edgeYTop = edgeYBottom + ceilingScale;

				if ((farPoint.y >= edgeYBottom) && (farPoint.y <= edgeYTop))
				{
					const double t = (farPoint - rayCoord.point).length();
					const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
					hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
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
			const double voxelHeight = isDryChasm ? ceilingScale : ArenaVoxelUtils::WET_CHASM_DEPTH;

			const double chasmYTop = static_cast<double>(voxel.y + 1) * ceilingScale;
			const double chasmYBottom = chasmYTop - voxelHeight;

			// See if the ray starts above or below the chasm floor.
			if (rayCoord.point.y >= chasmYBottom)
			{
				// Get any non-default state for this chasm voxel.
				const VoxelInstance::ChasmState *chasmState = [&voxel, &chunk]() -> const VoxelInstance::ChasmState*
				{
					const VoxelInstance *voxelInst = chunk->tryGetVoxelInst(voxel, VoxelInstance::Type::Chasm);
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
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						chasmYBottom,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
					return true;
				}
				else if ((farFacing != VoxelFacing3D::PositiveY) &&
					((chasmState != nullptr) && chasmState->faceIsVisible(farFacing)))
				{
					// Hits a side wall.
					const double t = (farPoint - rayCoord.point).length();
					const CoordDouble3 hitCoord(rayCoord.chunk, farPoint);
					hit.initVoxel(t, hitCoord, voxelID, voxel, &farFacing);
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
					const VoxelDouble3 planeOrigin(
						static_cast<SNDouble>(voxel.x) + 0.50,
						chasmYBottom,
						static_cast<WEDouble>(voxel.z) + 0.50);
					const VoxelDouble3 planeNormal = -Double3::UnitY;

					// Ray-plane intersection (guaranteed to hit a valid spot).
					VoxelDouble3 hitPoint;
					const bool success = MathUtils::rayPlaneIntersection(
						rayCoord.point, rayDirection, planeOrigin, planeNormal, &hitPoint);
					DebugAssert(success);
					CoordDouble3 hitCoord(rayCoord.chunk, hitPoint);

					const double t = (hitPoint - rayCoord.point).length();
					const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
					hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
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

	// Checks a voxel for ray hits and writes them into the output parameter. The near point and far point
	// are in the voxel coord's chunk, not necessarily the ray's. Returns true if the ray hit something.
	bool testVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const CoordInt3 &voxelCoord,
		VoxelFacing3D nearFacing, const CoordDouble3 &nearCoord, const CoordDouble3 &farCoord,
		double ceilingScale, const LevelInstance &levelInst, Physics::Hit &hit)
	{
		const ChunkManager &chunkManager = levelInst.getChunkManager();
		const Chunk *chunk = chunkManager.tryGetChunk(voxelCoord.chunk);
		if (chunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		const VoxelInt3 &voxel = voxelCoord.voxel;
		if (!chunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		const Chunk::VoxelID voxelID = chunk->getVoxel(voxel.x, voxel.y, voxel.z);

		// Get the voxel definition associated with the voxel.
		const VoxelDefinition &voxelDef = chunk->getVoxelDef(voxelID);
		const ArenaTypes::VoxelType voxelType = voxelDef.type;

		// Use absolute voxel when generating quads for ray intersection.
		const NewInt3 absoluteVoxel = VoxelUtils::coordToNewVoxel(voxelCoord);

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
			const double t = (nearCoord - rayCoord).length();
			hit.initVoxel(t, nearCoord, voxelID, voxel, &nearFacing);
			return true;
		}
		else if (voxelType == ArenaTypes::VoxelType::Floor)
		{
			// Intersect the floor as a quad.
			Quad quad;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
			NewDouble3 absoluteHitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);
			const CoordDouble3 hitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);

			if (success)
			{
				const double t = (hitCoord - rayCoord).length();
				const VoxelFacing3D facing = VoxelFacing3D::PositiveY;
				hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
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
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
			NewDouble3 absoluteHitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

			if (success)
			{
				const CoordDouble3 hitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
				const double t = (hitCoord - rayCoord).length();
				const VoxelFacing3D facing = VoxelFacing3D::NegativeY;
				hit.initVoxel(t, hitCoord, voxelID, voxel, &facing);
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
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale,
				nullptr, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			CoordDouble3 closestHitCoord;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
				NewDouble3 absoluteHitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

				if (success)
				{
					const double t = (absoluteHitPoint - absoluteRayPoint).length();
					if (t < closestT)
					{
						closestT = t;
						closestHitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
						closestIndex = i;
					}
				}
			}

			if (closestT < Hit::MAX_T)
			{
				const Quad &closestQuad = quads[closestIndex];
				const VoxelDouble3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::tryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, nullptr);
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
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale, nullptr, &quad, 1);
			DebugAssert(quadsWritten == 1);

			const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
			NewDouble3 absoluteHitPoint;
			const bool success = MathUtils::rayQuadIntersection(
				absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

			if (success)
			{
				const CoordDouble3 hitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
				const double t = (hitCoord - rayCoord).length();
				hit.initVoxel(t, hitCoord, voxelID, voxel, nullptr);
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
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale,
				nullptr, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			CoordDouble3 closestHitCoord;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
				NewDouble3 absoluteHitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

				if (success)
				{
					const double t = (absoluteHitPoint - absoluteRayPoint).length();
					if (t < closestT)
					{
						closestT = t;
						closestHitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
						closestIndex = i;
					}
				}
			}

			if (closestT < Hit::MAX_T)
			{
				const Quad &closestQuad = quads[closestIndex];
				const VoxelDouble3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::tryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, nullptr);
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
				const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale, nullptr, &quad, 1);
				DebugAssert(quadsWritten == 1);

				const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
				NewDouble3 absoluteHitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

				if (success)
				{
					const CoordDouble3 hitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
					const double t = (hitCoord - rayCoord).length();
					const VoxelFacing3D edgeFacing3D = VoxelUtils::convertFaceTo3D(edge.facing);
					hit.initVoxel(t, hitCoord, voxelID, voxel, &edgeFacing3D);
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
			const VoxelInstance *voxelInst = chunk->tryGetVoxelInst(voxel, VoxelInstance::Type::Chasm);

			// Intersect each face and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, voxelInst, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale,
				voxelInst, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			CoordDouble3 closestHitCoord;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
				NewDouble3 absoluteHitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

				if (success)
				{
					const double t = (absoluteHitPoint - absoluteRayPoint).length();
					if (t < closestT)
					{
						closestT = t;
						closestHitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
						closestIndex = i;
					}
				}
			}

			if (closestT < Hit::MAX_T)
			{
				const Quad &closestQuad = quads[closestIndex];
				const VoxelDouble3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::tryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, nullptr);
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

			const VoxelInstance *voxelInst = chunk->tryGetVoxelInst(voxel, VoxelInstance::Type::OpenDoor);

			// Intersect each face and find the closest one (if any).
			int quadCount;
			VoxelGeometry::getInfo(voxelDef, voxelInst, &quadCount);

			std::array<Quad, VoxelGeometry::MAX_QUADS> quads;
			const int quadsWritten = VoxelGeometry::getQuads(voxelDef, absoluteVoxel, ceilingScale,
				voxelInst, quads.data(), static_cast<int>(quads.size()));
			DebugAssert(quadsWritten == quadCount);

			double closestT = Hit::MAX_T;
			CoordDouble3 closestHitCoord;
			int closestIndex;

			for (int i = 0; i < quadsWritten; i++)
			{
				const Quad &quad = quads[i];

				const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayCoord);
				NewDouble3 absoluteHitPoint;
				const bool success = MathUtils::rayQuadIntersection(
					absoluteRayPoint, rayDirection, quad.getV0(), quad.getV1(), quad.getV2(), &absoluteHitPoint);

				if (success)
				{
					const double t = (absoluteHitPoint - absoluteRayPoint).length();
					if (t < closestT)
					{
						closestT = t;
						closestHitCoord = VoxelUtils::newPointToCoord(absoluteHitPoint);
						closestIndex = i;
					}
				}
			}

			if (closestT < Hit::MAX_T)
			{
				const Quad &closestQuad = quads[closestIndex];
				const VoxelDouble3 normal = closestQuad.getNormal();
				VoxelFacing3D facing;
				if (Physics::tryGetFacingFromNormal(normal, &facing))
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, &facing);
				}
				else
				{
					hit.initVoxel(closestT, closestHitCoord, voxelID, voxel, nullptr);
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
	bool testEntitiesInVoxel(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection,
		const VoxelDouble3 &flatForward, const VoxelDouble3 &flatRight, const VoxelDouble3 &flatUp,
		const VoxelInt3 &voxel, const ChunkEntityMap &chunkEntityMap, bool pixelPerfect, const Palette &palette,
		const EntityManager &entityManager, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit)
	{
		// Use a separate hit variable so we can determine whether an entity was closer.
		Physics::Hit entityHit;
		entityHit.setT(Hit::MAX_T);

		const auto &entityMappings = chunkEntityMap.mappings;
		const auto iter = entityMappings.find(voxel);
		if (iter != entityMappings.end())
		{
			// Iterate over all the entities that cross this voxel and ray test them.
			const std::vector<EntityManager::EntityVisibilityState3D> &entityVisStateList = iter->second;
			for (const EntityManager::EntityVisibilityState3D &visState : entityVisStateList)
			{
				const Entity &entity = *visState.entity;
				const EntityDefinition &entityDef = entityManager.getEntityDef(
					entity.getDefinitionID(), entityDefLibrary);
				const EntityAnimationDefinition::Keyframe &animKeyframe =
					entityManager.getEntityAnimKeyframe(entity, visState, entityDefLibrary);

				const double flatWidth = animKeyframe.getWidth();
				const double flatHeight = animKeyframe.getHeight();

				CoordDouble3 hitCoord;
				if (renderer.getEntityRayIntersection(visState, entityDef, flatForward, flatRight, flatUp,
					flatWidth, flatHeight, rayCoord, rayDirection, pixelPerfect, palette, &hitCoord))
				{
					const double distance = (hitCoord - rayCoord).length();
					if (distance < entityHit.getT())
					{
						entityHit.initEntity(distance, hitCoord, entity.getID(), entity.getEntityType());
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

	// Internal ray casting loop for stepping through individual voxels and checking ray intersections
	// against voxels and entities.
	template <bool NonNegativeDirX, bool NonNegativeDirY, bool NonNegativeDirZ>
	void rayCastInternal(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection,
		const VoxelDouble3 &cameraForward, double ceilingScale, const LevelInstance &levelInst, bool pixelPerfect,
		bool includeEntities, const Palette &palette, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, std::vector<ChunkEntityMap> &chunkEntityMaps, Physics::Hit &hit)
	{
		const ChunkManager &chunkManager = levelInst.getChunkManager();
		const EntityManager &entityManager = levelInst.getEntityManager();

		// Each flat shares the same axes. Their forward direction always faces opposite to the camera direction.
		const VoxelDouble3 flatForward = VoxelDouble3(-cameraForward.x, 0.0, -cameraForward.z).normalized();
		const VoxelDouble3 flatUp = Double3::UnitY;
		const VoxelDouble3 flatRight = flatForward.cross(flatUp).normalized();

		// Axis length is the length of a voxel in each dimension (required for tall voxels).
		const VoxelDouble3 axisLen(1.0, ceilingScale, 1.0);

		// Initial voxel as reals and integers.
		const VoxelDouble3 rayVoxelReal(
			std::floor(rayCoord.point.x / axisLen.x),
			std::floor(rayCoord.point.y / axisLen.y),
			std::floor(rayCoord.point.z / axisLen.z));
		const VoxelInt3 rayVoxel(
			static_cast<SNInt>(rayVoxelReal.x),
			static_cast<int>(rayVoxelReal.y),
			static_cast<WEInt>(rayVoxelReal.z));

		// World space (not grid space) floor of the voxel the ray starts in.
		const VoxelDouble3 rayPointWorldFloor = rayVoxelReal * axisLen;

		// Delta distance is how far the ray has to go to step one voxel's worth along a certain axis.
		// This is affected by grid properties like tall voxels.
		const VoxelDouble3 deltaDist(
			(NonNegativeDirX ? axisLen.x : -axisLen.x) / rayDirection.x,
			(NonNegativeDirY ? axisLen.y : -axisLen.y) / rayDirection.y,
			(NonNegativeDirZ ? axisLen.z : -axisLen.z) / rayDirection.z);

		DebugAssert(deltaDist.x >= 0.0);
		DebugAssert(deltaDist.y >= 0.0);
		DebugAssert(deltaDist.z >= 0.0);

		// Step is the voxel delta per step (always +/- 1), also usable when updating the chunk coordinate.
		// The initial delta distances are percentages of the delta distances, dependent on the ray start
		// position inside the voxel.
		constexpr SNInt stepX = NonNegativeDirX ? 1 : -1;
		constexpr int stepY = NonNegativeDirY ? 1 : -1;
		constexpr WEInt stepZ = NonNegativeDirZ ? 1 : -1;

		const SNDouble initialDeltaDistPercentX = NonNegativeDirX ?
			(1.0 - ((rayCoord.point.x - rayPointWorldFloor.x) / axisLen.x)) :
			((rayCoord.point.x - rayPointWorldFloor.x) / axisLen.x);
		const double initialDeltaDistPercentY = NonNegativeDirY ?
			(1.0 - ((rayCoord.point.y - rayPointWorldFloor.y) / axisLen.y)) :
			((rayCoord.point.y - rayPointWorldFloor.y) / axisLen.y);
		const WEDouble initialDeltaDistPercentZ = NonNegativeDirZ ?
			(1.0 - ((rayCoord.point.z - rayPointWorldFloor.z) / axisLen.z)) :
			((rayCoord.point.z - rayPointWorldFloor.z) / axisLen.z);

		DebugAssert(initialDeltaDistPercentX >= 0.0);
		DebugAssert(initialDeltaDistPercentX <= 1.0);
		DebugAssert(initialDeltaDistPercentY >= 0.0);
		DebugAssert(initialDeltaDistPercentY <= 1.0);
		DebugAssert(initialDeltaDistPercentZ >= 0.0);
		DebugAssert(initialDeltaDistPercentZ <= 1.0);

		// Initial delta distance is a fraction of delta distance based on the ray's position in
		// the initial voxel.
		const SNDouble initialDeltaDistX = deltaDist.x * initialDeltaDistPercentX;
		const double initialDeltaDistY = deltaDist.y * initialDeltaDistPercentY;
		const WEDouble initialDeltaDistZ = deltaDist.z * initialDeltaDistPercentZ;

		// The visible voxel facings for each axis depending on ray direction. The facing is opposite
		// to the direction (i.e. negative Y face if stepping upward).
		constexpr std::array<VoxelFacing3D, 3> visibleWallFacings =
		{
			NonNegativeDirX ? VoxelFacing3D::NegativeX : VoxelFacing3D::PositiveX,
			NonNegativeDirY ? VoxelFacing3D::NegativeY : VoxelFacing3D::PositiveY,
			NonNegativeDirZ ? VoxelFacing3D::NegativeZ : VoxelFacing3D::PositiveZ
		};

		// The current ray distance and intersected face of the voxel.
		double rayDistance;
		VoxelFacing3D facing;

		// Check whether the initial voxel is in a loaded chunk.
		ChunkInt2 currentChunk = rayCoord.chunk;
		const Chunk *currentChunkPtr = chunkManager.tryGetChunk(currentChunk);

		// The initial DDA step is a special case, so it's brought outside the DDA loop. This complicates things
		// a little bit, but it's important enough that it should be kept.
		if (currentChunkPtr != nullptr)
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

			// The initial far point is the voxel hit point.
			const VoxelDouble3 initialFarPoint = rayCoord.point + (rayDirection * rayDistance);

			// Test the initial voxel's geometry for ray intersections.
			bool success = Physics::testInitialVoxelRay(rayCoord, rayDirection, rayVoxel, facing,
				initialFarPoint, ceilingScale, levelInst, hit);

			if (includeEntities)
			{
				// Test the initial voxel's entities for ray intersections.
				const ChunkEntityMap &chunkEntityMap = Physics::getOrAddChunkEntityMap(currentChunk, rayCoord,
					ceilingScale, chunkManager, entityManager, entityDefLibrary, chunkEntityMaps);
				success |= Physics::testEntitiesInVoxel(rayCoord, rayDirection, flatForward, flatRight, flatUp,
					rayVoxel, chunkEntityMap, pixelPerfect, palette, entityManager, entityDefLibrary, renderer, hit);
			}

			if (success)
			{
				// The ray hit something in the initial voxel.
				return;
			}
		}

		// The current voxel coordinate in the DDA loop.
		VoxelInt3 currentVoxel = rayVoxel;

		// Delta distance sums in each component, starting at the initial wall hit. The lowest component is
		// the candidate for the next DDA loop.
		SNDouble deltaDistSumX = initialDeltaDistX;
		double deltaDistSumY = initialDeltaDistY;
		WEDouble deltaDistSumZ = initialDeltaDistZ;

		// We do need an exit condition in case Y stepping would result in never being in the chunk, since it doesn't
		// follow the same wrapping rule as X and Z. Doing this instead of "is voxel Y valid?" lets the player be
		// above or below the chunk and still select things.
		bool canDoYStep = (currentChunkPtr != nullptr) && 
			(NonNegativeDirY ? (currentVoxel.y < currentChunkPtr->getHeight()) : (currentVoxel.y >= 0));

		// Helper values for ray distance calculation.
		constexpr SNDouble halfOneMinusStepXReal = static_cast<SNDouble>((1 - stepX) / 2);
		constexpr double halfOneMinusStepYReal = static_cast<double>((1 - stepY) / 2);
		constexpr WEDouble halfOneMinusStepZReal = static_cast<WEDouble>((1 - stepZ) / 2);

		// Lambda for stepping to the next voxel in the grid and updating various values.
		auto doDDAStep = [&rayCoord, &rayDirection, &chunkManager, &deltaDist, stepX, stepY, stepZ, initialDeltaDistX,
			initialDeltaDistY, initialDeltaDistZ, &visibleWallFacings, &rayDistance, &facing, &currentChunk,
			&currentChunkPtr, &currentVoxel, &deltaDistSumX, &deltaDistSumY, &deltaDistSumZ, &canDoYStep,
			halfOneMinusStepXReal, halfOneMinusStepYReal, halfOneMinusStepZReal]()
		{
			const ChunkInt2 oldChunk = currentChunk;

			if ((deltaDistSumX < deltaDistSumY) && (deltaDistSumX < deltaDistSumZ))
			{
				deltaDistSumX += deltaDist.x;
				currentVoxel.x += stepX;

				if (NonNegativeDirX)
				{
					if (currentVoxel.x >= ChunkUtils::CHUNK_DIM)
					{
						currentVoxel.x = 0;
						currentChunk.x++;
					}
				}
				else
				{
					if (currentVoxel.x < 0)
					{
						currentVoxel.x = ChunkUtils::CHUNK_DIM - 1;
						currentChunk.x--;
					}
				}

				facing = visibleWallFacings[0];

				const SNDouble combinedStepDistX = static_cast<SNDouble>(currentVoxel.x) +
					static_cast<SNDouble>((currentChunk.x - rayCoord.chunk.x) * ChunkUtils::CHUNK_DIM);
				rayDistance = ((combinedStepDistX - rayCoord.point.x) + halfOneMinusStepXReal) / rayDirection.x;
			}
			else if (deltaDistSumY < deltaDistSumZ)
			{
				deltaDistSumY += deltaDist.y;
				currentVoxel.y += stepY;
				canDoYStep = NonNegativeDirY ? (currentVoxel.y < currentChunkPtr->getHeight()) : (currentVoxel.y >= 0);
				facing = visibleWallFacings[1];
				rayDistance = ((static_cast<double>(currentVoxel.y) - rayCoord.point.y) + halfOneMinusStepYReal) / rayDirection.y;
			}
			else
			{
				deltaDistSumZ += deltaDist.z;
				currentVoxel.z += stepZ;

				if (NonNegativeDirZ)
				{
					if (currentVoxel.z >= ChunkUtils::CHUNK_DIM)
					{
						currentVoxel.z = 0;
						currentChunk.y++;
					}
				}
				else
				{
					if (currentVoxel.z < 0)
					{
						currentVoxel.z = ChunkUtils::CHUNK_DIM - 1;
						currentChunk.y--;
					}
				}

				facing = visibleWallFacings[2];

				const WEDouble combinedStepDistZ = static_cast<WEDouble>(currentVoxel.z) +
					static_cast<WEDouble>((currentChunk.y - rayCoord.chunk.y) * ChunkUtils::CHUNK_DIM);
				rayDistance = ((combinedStepDistZ - rayCoord.point.z) + halfOneMinusStepZReal) / rayDirection.z;
			}

			if (currentChunk != oldChunk)
			{
				currentChunkPtr = chunkManager.tryGetChunk(currentChunk);
			}
		};

		// Step forward in the grid once to leave the initial voxel and update the ray distance.
		doDDAStep();

		// Step through the grid while the current chunk is valid and the Y voxel is valid (this needs its own check
		// since Y doesn't follow the same wrapping as X and Z). There doesn't need to be a max distance check.
		while ((currentChunkPtr != nullptr) && canDoYStep)
		{
			// Store part of the current DDA state. The loop needs to do another DDA step to calculate
			// the point on the far side of this voxel.
			const CoordInt3 savedVoxelCoord(currentChunk, currentVoxel);
			const VoxelFacing3D savedFacing = facing;
			const double savedDistance = rayDistance;

			// Decide which voxel to step to next, and update the ray distance.
			doDDAStep();

			// Near and far points in the voxel. The near point is where the voxel was hit before, and the far
			// point is where the voxel was just hit on the far side.
			const CoordDouble3 nearCoord = ChunkUtils::recalculateCoord(
				rayCoord.chunk, rayCoord.point + (rayDirection * savedDistance));
			const CoordDouble3 farCoord = ChunkUtils::recalculateCoord(
				rayCoord.chunk, rayCoord.point + (rayDirection * rayDistance));

			// Test the current voxel's geometry for ray intersections.
			bool success = Physics::testVoxelRay(rayCoord, rayDirection, savedVoxelCoord, savedFacing,
				nearCoord, farCoord, ceilingScale, levelInst, hit);

			if (includeEntities)
			{
				// Test the current voxel's entities for ray intersections.
				const ChunkEntityMap &chunkEntityMap = Physics::getOrAddChunkEntityMap(savedVoxelCoord.chunk, rayCoord,
					ceilingScale, chunkManager, entityManager, entityDefLibrary, chunkEntityMaps);
				success |= Physics::testEntitiesInVoxel(rayCoord, rayDirection, flatForward, flatRight, flatUp,
					savedVoxelCoord.voxel, chunkEntityMap, pixelPerfect, palette, entityManager, entityDefLibrary,
					renderer, hit);
			}

			if (success)
			{
				// The ray hit something in a voxel.
				break;
			}
		}
	}
}

void Physics::Hit::initVoxel(double t, const CoordDouble3 &coord, uint16_t id, const VoxelInt3 &voxel,
	const VoxelFacing3D *facing)
{
	this->t = t;
	this->coord = coord;
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

void Physics::Hit::initEntity(double t, const CoordDouble3 &coord, EntityID id, EntityType type)
{
	this->t = t;
	this->coord = coord;
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

const CoordDouble3 &Physics::Hit::getCoord() const
{
	return this->coord;
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

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, double ceilingScale,
	const VoxelDouble3 &cameraForward, bool pixelPerfect, const Palette &palette, bool includeEntities,
	const LevelInstance &levelInst, const EntityDefinitionLibrary &entityDefLibrary,
	const Renderer &renderer, Physics::Hit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.setT(Hit::MAX_T);

	// Voxel->entity mappings for each chunk touched by the ray casting loop.
	std::vector<ChunkEntityMap> chunkEntityMaps;

	// Ray cast through the voxel grid, populating the output hit data. Use the ray direction booleans for
	// better code generation (at the expense of having a pile of if/else branches here).
	const bool nonNegativeDirX = rayDirection.x >= 0.0;
	const bool nonNegativeDirY = rayDirection.y >= 0.0;
	const bool nonNegativeDirZ = rayDirection.z >= 0.0;

	if (nonNegativeDirX)
	{
		if (nonNegativeDirY)
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, true, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
			else
			{
				Physics::rayCastInternal<true, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
			else
			{
				Physics::rayCastInternal<true, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
		}
	}
	else
	{
		if (nonNegativeDirY)
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, true, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
			else
			{
				Physics::rayCastInternal<false, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
			else
			{
				Physics::rayCastInternal<false, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					levelInst, pixelPerfect, includeEntities, palette, entityDefLibrary, renderer, chunkEntityMaps,
					hit);
			}
		}
	}

	// Return whether the ray hit something.
	return hit.getT() < Hit::MAX_T;
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection,
	const VoxelDouble3 &cameraForward, bool pixelPerfect, const Palette &palette, bool includeEntities,
	const LevelInstance &levelInst, const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer,
	Physics::Hit &hit)
{
	constexpr double ceilingScale = 1.0;
	return Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraForward, pixelPerfect, palette,
		includeEntities, levelInst, entityDefLibrary, renderer, hit);
}
