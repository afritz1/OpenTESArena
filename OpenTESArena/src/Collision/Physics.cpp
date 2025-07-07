#include <algorithm>
#include <cmath>

#include "Physics.h"
#include "RayCastTypes.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/MIFFile.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityObservedResult.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelFacing.h"
#include "../World/ChunkUtils.h"
#include "../World/MeshUtils.h"

#include "components/debug/Debug.h"

namespace Physics
{
	struct EntityEntry
	{
		CoordDouble3 coord;
		EntityObservedResult observedResult;

		void init(const CoordDouble3 &coord, const EntityObservedResult &observedResult)
		{
			this->coord = coord;
			this->observedResult = observedResult;
		}
	};

	// Container of the voxels each entity is touching per chunk. Each chunk needs to look at adjacent chunk
	// entities in case some of them overlap the chunk edge.
	struct ChunkEntityMap
	{
		ChunkInt2 chunk;
		std::unordered_map<VoxelInt3, std::vector<EntityEntry>> mappings;

		void init(const ChunkInt2 &chunk)
		{
			this->chunk = chunk;
		}

		void add(const VoxelInt3 &voxel, const EntityEntry &entry)
		{
			auto iter = this->mappings.find(voxel);
			if (iter == this->mappings.end())
			{
				iter = this->mappings.emplace(voxel, std::vector<EntityEntry>()).first;
			}

			std::vector<EntityEntry> &entryList = iter->second;
			entryList.emplace_back(entry);
		}
	};

	// Builds a set of voxels for a chunk that are at least partially touched by entities. A point of reference
	// is needed for evaluating entity animations. Ignores entities behind the camera.
	Physics::ChunkEntityMap makeChunkEntityMap(const ChunkInt2 &chunk, const CoordDouble3 &viewCoord,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary)
	{
		// Include entities within one chunk of the center chunk to get entities that are partially touching
		// the center chunk.
		constexpr int chunkDistance = 1;
		ChunkInt2 minChunk, maxChunk;
		ChunkUtils::getSurroundingChunks(chunk, chunkDistance, &minChunk, &maxChunk);

		// Gather up entities in nearby chunks.
		const int totalNearbyEntities = [&entityChunkManager, &minChunk, &maxChunk]()
		{
			int count = 0;
			for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
			{
				for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
				{
					const EntityChunk *entityChunkPtr = entityChunkManager.findChunkAtPosition(ChunkInt2(x, z));
					if (entityChunkPtr != nullptr)
					{
						count += static_cast<int>(entityChunkPtr->entityIDs.size());
					}
				}
			}

			return count;
		}();

		Buffer<EntityInstanceID> entityInstIDs(totalNearbyEntities);
		entityInstIDs.fill(-1);

		int entityInsertIndex = 0;
		auto addEntitiesFromChunk = [&entityChunkManager, &entityInstIDs, &entityInsertIndex](SNInt chunkX, WEInt chunkZ)
		{
			EntityInstanceID *entityInstIDsPtr = entityInstIDs.begin() + entityInsertIndex;
			const int size = entityInstIDs.getCount() - entityInsertIndex;
			const EntityChunk *entityChunkPtr = entityChunkManager.findChunkAtPosition(ChunkInt2(chunkX, chunkZ));
			if (entityChunkPtr != nullptr)
			{
				int writtenCount = 0;
				for (size_t i = 0; i < entityChunkPtr->entityIDs.size(); i++)
				{
					if (writtenCount >= size)
					{
						break;
					}

					entityInstIDsPtr[i] = entityChunkPtr->entityIDs[i];
					writtenCount++;
				}

				DebugAssert(writtenCount <= size);
				entityInsertIndex += writtenCount;
			}
		};

		for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
		{
			for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
			{
				addEntitiesFromChunk(x, z);
			}
		}

		const WorldDouble3 viewPosition = VoxelUtils::coordToWorldPoint(viewCoord);

		ChunkEntityMap chunkEntityMap;
		chunkEntityMap.init(chunk);

		// Build mappings of voxels to entities.
		for (const EntityInstanceID entityInstID : entityInstIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);

			EntityObservedResult observedResult;
			entityChunkManager.getEntityObservedResult(entityInstID, viewPosition, observedResult);

			// Iterate over the voxels the entity's bounding box touches.
			const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInstID);
			const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
			const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
			const WorldDouble3 entityMinWorldPoint = entityPosition - Double3(entityBBox.halfWidth, 0.0, entityBBox.halfDepth);
			const WorldDouble3 entityMaxWorldPoint = entityPosition + Double3(entityBBox.halfWidth, entityBBox.height, entityBBox.halfDepth);
			const WorldInt3 entityMinWorldVoxel = VoxelUtils::pointToVoxel(entityMinWorldPoint, ceilingScale);
			const WorldInt3 entityMaxWorldVoxel = VoxelUtils::pointToVoxel(entityMaxWorldPoint, ceilingScale);
			const WorldInt3 voxelCoordDiff = entityMaxWorldVoxel - entityMinWorldVoxel;

			for (WEInt z = 0; z <= voxelCoordDiff.z; z++)
			{
				for (int y = 0; y <= voxelCoordDiff.y; y++)
				{
					for (SNInt x = 0; x <= voxelCoordDiff.x; x++)
					{
						const WorldInt3 curWorldVoxel(entityMinWorldVoxel.x + x, entityMinWorldVoxel.y + y, entityMinWorldVoxel.z + z);
						const CoordInt3 curCoord = VoxelUtils::worldVoxelToCoord(curWorldVoxel);

						if (curCoord.chunk == chunk)
						{
							EntityEntry entry;
							entry.init(entityCoord, observedResult);
							chunkEntityMap.add(curCoord.voxel, entry);
						}
					}
				}
			}
		}

		return chunkEntityMap;
	}

	// The given chunk coordinate is known to be loaded.
	const ChunkEntityMap &getOrAddChunkEntityMap(const ChunkInt2 &chunk, const CoordDouble3 &viewCoord,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary, std::vector<ChunkEntityMap> &chunkEntityMaps)
	{
		for (const ChunkEntityMap &map : chunkEntityMaps)
		{
			if (map.chunk == chunk)
			{
				return map;
			}
		}

		ChunkEntityMap newMap = Physics::makeChunkEntityMap(chunk, viewCoord, ceilingScale, voxelChunkManager,
			entityChunkManager, entityDefLibrary);
		chunkEntityMaps.emplace_back(std::move(newMap));
		return chunkEntityMaps.back();
	}

	bool getEntityRayIntersection(const EntityObservedResult &observedResult, const CoordDouble3 &entityCoord, const EntityDefinition &entityDef,
		const VoxelDouble3 &entityForward, const VoxelDouble3 &entityRight, const VoxelDouble3 &entityUp, double entityWidth, double entityHeight,
		const WorldDouble3 &rayWorldPoint, const VoxelDouble3 &rayDirection, WorldDouble3 *outHitPoint)
	{
		const WorldDouble3 absoluteFlatPosition = VoxelUtils::coordToWorldPoint(entityCoord);

		double hitT;
		if (!MathUtils::rayPlaneIntersection(rayWorldPoint, rayDirection, absoluteFlatPosition, entityForward, &hitT))
		{
			// Did not intersect the entity's plane.
			return false;
		}

		const WorldDouble3 absoluteHitPoint = rayWorldPoint + (rayDirection * hitT);
		const WorldDouble3 diff = absoluteHitPoint - absoluteFlatPosition;

		// Get the texture coordinates. It's okay if they are outside the entity.
		const Double2 uv(
			0.5 - (diff.dot(entityRight) / entityWidth),
			1.0 - (diff.dot(entityUp) / entityHeight));

		const EntityAnimationDefinition &animDef = entityDef.animDef;
		const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;
		DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
		const EntityAnimationDefinitionKeyframe &animKeyframe = animDef.keyframes[linearizedKeyframeIndex];
		const TextureAsset &textureAsset = animKeyframe.textureAsset;

		// The entity's projected rectangle is hit if the texture coordinates are valid.
		const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);
		if (!withinEntity)
		{
			return false;
		}

		*outHitPoint = absoluteHitPoint;
		return true;
	}

	// @todo: use Jolt instead
	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	bool testInitialVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const VoxelInt3 &voxel,
		VoxelFacing3D farFacing, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const CollisionChunkManager &collisionChunkManager, RayCastHit &hit)
	{
		const ChunkInt2 chunkPos = rayCoord.chunk;
		const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(chunkPos);
		if (voxelChunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		if (!voxelChunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		const CollisionChunk *collisionChunk = collisionChunkManager.findChunkAtPosition(chunkPos);
		DebugAssert(collisionChunk != nullptr);
		if (!collisionChunk->enabledColliders.get(voxel.x, voxel.y, voxel.z))
		{
			// Collider is not turned on.
			return false;
		}

		const VoxelShapeDefID voxelShapeDefID = voxelChunk->shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk->shapeDefs[voxelShapeDefID];
		const VoxelShapeScaleType scaleType = voxelShapeDef.scaleType;

		const CollisionShapeDefID collisionShapeDefID = collisionChunk->shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const CollisionShapeDefinition &collisionShapeDef = collisionChunk->getCollisionShapeDef(collisionShapeDefID);
		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &collisionBoxShapeDef = collisionShapeDef.box;

		const WorldInt3 worldVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, voxel);
		const WorldDouble3 worldVoxelReal(
			static_cast<SNDouble>(worldVoxel.x),
			static_cast<double>(worldVoxel.y) * ceilingScale,
			static_cast<WEDouble>(worldVoxel.z));
		const WorldDouble3 collisionBoxShapeCenter(
			worldVoxelReal.x + 0.50,
			worldVoxelReal.y + MeshUtils::getScaledVertexY(collisionBoxShapeDef.yOffset + (collisionBoxShapeDef.height * 0.50), scaleType, ceilingScale),
			worldVoxelReal.z + 0.50);
		const double collisionBoxShapeScaledHeight = collisionBoxShapeDef.height * ceilingScale;
		const WorldDouble3 worldRayStart = VoxelUtils::coordToWorldPoint(rayCoord);

		double hitT;
		if (!MathUtils::rayBoxIntersection(worldRayStart, rayDirection, collisionBoxShapeCenter, collisionBoxShapeDef.width, collisionBoxShapeScaledHeight,
			collisionBoxShapeDef.depth, collisionBoxShapeDef.yRotation, &hitT))
		{
			return false;
		}

		const WorldDouble3 hitWorldPoint = worldRayStart + (rayDirection * hitT);
		const CoordInt3 voxelCoord(chunkPos, voxel);
		hit.initVoxel(hitT, hitWorldPoint, voxelCoord, farFacing);
		return true;
	}

	// @todo: use Jolt instead
	// Checks a voxel for ray hits and writes them into the output parameter. The near point and far point
	// are in the voxel coord's chunk, not necessarily the ray's. Returns true if the ray hit something.
	bool testVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const CoordInt3 &voxelCoord,
		VoxelFacing3D nearFacing, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const CollisionChunkManager &collisionChunkManager, RayCastHit &hit)
	{
		const ChunkInt2 chunkPos = voxelCoord.chunk;
		const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(chunkPos);
		if (voxelChunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		const VoxelInt3 voxel = voxelCoord.voxel;
		if (!voxelChunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		const CollisionChunk *collisionChunk = collisionChunkManager.findChunkAtPosition(chunkPos);
		DebugAssert(collisionChunk != nullptr);
		if (!collisionChunk->enabledColliders.get(voxel.x, voxel.y, voxel.z))
		{
			// Collider is not turned on.
			return false;
		}

		const VoxelShapeDefID voxelShapeDefID = voxelChunk->shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk->shapeDefs[voxelShapeDefID];
		const VoxelShapeScaleType scaleType = voxelShapeDef.scaleType;

		const CollisionShapeDefID collisionShapeDefID = collisionChunk->shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const CollisionShapeDefinition &collisionShapeDef = collisionChunk->getCollisionShapeDef(collisionShapeDefID);
		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &collisionBoxShapeDef = collisionShapeDef.box;

		const WorldInt3 worldVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, voxel);
		const WorldDouble3 worldVoxelReal(
			static_cast<SNDouble>(worldVoxel.x),
			static_cast<double>(worldVoxel.y) * ceilingScale,
			static_cast<WEDouble>(worldVoxel.z));
		const WorldDouble3 collisionBoxShapeCenter(
			worldVoxelReal.x + 0.50,
			worldVoxelReal.y + MeshUtils::getScaledVertexY(collisionBoxShapeDef.yOffset + (collisionBoxShapeDef.height * 0.50), scaleType, ceilingScale),
			worldVoxelReal.z + 0.50);
		const double collisionBoxShapeScaledHeight = collisionBoxShapeDef.height * ceilingScale;
		const WorldDouble3 worldRayStart = VoxelUtils::coordToWorldPoint(rayCoord);

		double hitT;
		if (!MathUtils::rayBoxIntersection(worldRayStart, rayDirection, collisionBoxShapeCenter, collisionBoxShapeDef.width, collisionBoxShapeScaledHeight,
			collisionBoxShapeDef.depth, collisionBoxShapeDef.yRotation, &hitT))
		{
			return false;
		}

		const WorldDouble3 hitWorldPoint = worldRayStart + (rayDirection * hitT);
		hit.initVoxel(hitT, hitWorldPoint, voxelCoord, nearFacing);
		return true;
	}

	// Helper function for testing which entities in a voxel are intersected by a ray.
	bool testEntitiesInVoxel(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection,
		const VoxelDouble3 &flatForward, const VoxelDouble3 &flatRight, const VoxelDouble3 &flatUp,
		const VoxelInt3 &voxel, const ChunkEntityMap &chunkEntityMap, const EntityChunkManager &entityChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary, RayCastHit &hit)
	{
		const WorldDouble3 rayWorldPoint = VoxelUtils::coordToWorldPoint(rayCoord); // @todo just use WorldDouble3 everywhere?

		// Use a separate hit variable so we can determine whether an entity was closer.
		RayCastHit entityHit;
		entityHit.t = RayCastHit::NO_HIT_DISTANCE;

		const auto &entityMappings = chunkEntityMap.mappings;
		const auto iter = entityMappings.find(voxel);
		if (iter != entityMappings.end())
		{
			// Iterate over all entities that cross this voxel and ray test them.
			Span<const EntityEntry> entityEntryList = iter->second;
			for (const EntityEntry &entry : entityEntryList)
			{
				const EntityObservedResult &observedResult = entry.observedResult;
				const EntityInstanceID entityInstID = observedResult.entityInstID;
				const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;

				const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
				const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
				const EntityAnimationDefinition &animDef = entityDef.animDef;
				DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
				const EntityAnimationDefinitionKeyframe &animKeyframe = animDef.keyframes[linearizedKeyframeIndex];
				const double flatWidth = animKeyframe.width;
				const double flatHeight = animKeyframe.height;

				WorldDouble3 hitWorldPoint;
				if (Physics::getEntityRayIntersection(observedResult, entry.coord, entityDef, flatForward, flatRight, flatUp,
					flatWidth, flatHeight, rayWorldPoint, rayDirection, &hitWorldPoint))
				{
					const double distance = (hitWorldPoint - rayWorldPoint).length();
					if (distance < entityHit.t)
					{
						entityHit.initEntity(distance, hitWorldPoint, entityInstID);
					}
				}
			}
		}

		const bool entityIsCloser = entityHit.t < hit.t;
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
	template<bool NonNegativeDirX, bool NonNegativeDirY, bool NonNegativeDirZ>
	void rayCastInternal(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const VoxelDouble3 &cameraForward,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
		const CollisionChunkManager &collisionChunkManager, bool includeEntities, const EntityDefinitionLibrary &entityDefLibrary,
		std::vector<ChunkEntityMap> &chunkEntityMaps, RayCastHit &hit)
	{
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
		Double3 deltaDist(
			(NonNegativeDirX ? axisLen.x : -axisLen.x) / rayDirection.x,
			(NonNegativeDirY ? axisLen.y : -axisLen.y) / rayDirection.y,
			(NonNegativeDirZ ? axisLen.z : -axisLen.z) / rayDirection.z);

		// Filter bad ray directions like direction.y == 0 (at horizon) so that axis isn't selected during stepping.
		if (deltaDist.x < 0.0)
		{
			deltaDist.x = std::numeric_limits<double>::infinity();
		}

		if (deltaDist.y < 0.0)
		{
			deltaDist.y = std::numeric_limits<double>::infinity();
		}

		if (deltaDist.z < 0.0)
		{
			deltaDist.z = std::numeric_limits<double>::infinity();
		}

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
		const VoxelChunk *currentChunkPtr = voxelChunkManager.findChunkAtPosition(currentChunk);

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
				ceilingScale, voxelChunkManager, collisionChunkManager, hit);

			if (includeEntities)
			{
				// Test the initial voxel's entities for ray intersections.
				const ChunkEntityMap &chunkEntityMap = Physics::getOrAddChunkEntityMap(currentChunk, rayCoord,
					ceilingScale, voxelChunkManager, entityChunkManager, entityDefLibrary, chunkEntityMaps);
				success |= Physics::testEntitiesInVoxel(rayCoord, rayDirection, flatForward, flatRight, flatUp,
					rayVoxel, chunkEntityMap, entityChunkManager, entityDefLibrary, hit);
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
		bool canDoYStep = (currentChunkPtr != nullptr) && (NonNegativeDirY ? (currentVoxel.y < currentChunkPtr->height) : (currentVoxel.y >= 0));

		// Helper values for ray distance calculation.
		constexpr SNDouble halfOneMinusStepXReal = static_cast<SNDouble>((1 - stepX) / 2);
		constexpr double halfOneMinusStepYReal = static_cast<double>((1 - stepY) / 2);
		constexpr WEDouble halfOneMinusStepZReal = static_cast<WEDouble>((1 - stepZ) / 2);

		// Lambda for stepping to the next voxel in the grid and updating various values.
		auto doDDAStep = [&rayCoord, &rayDirection, &voxelChunkManager, &deltaDist, stepX, stepY, stepZ, initialDeltaDistX,
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
				canDoYStep = NonNegativeDirY ? (currentVoxel.y < currentChunkPtr->height) : (currentVoxel.y >= 0);
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
				currentChunkPtr = voxelChunkManager.findChunkAtPosition(currentChunk);
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
			//const CoordDouble3 nearCoord = ChunkUtils::recalculateCoord(rayCoord.chunk, rayCoord.point + (rayDirection * savedDistance));
			//const CoordDouble3 farCoord = ChunkUtils::recalculateCoord(rayCoord.chunk, rayCoord.point + (rayDirection * rayDistance));

			// Test the current voxel's geometry for ray intersections.
			bool success = Physics::testVoxelRay(rayCoord, rayDirection, savedVoxelCoord, savedFacing, ceilingScale,
				voxelChunkManager, collisionChunkManager, hit);

			if (includeEntities)
			{
				// Test the current voxel's entities for ray intersections.
				const ChunkEntityMap &chunkEntityMap = Physics::getOrAddChunkEntityMap(savedVoxelCoord.chunk, rayCoord,
					ceilingScale, voxelChunkManager, entityChunkManager, entityDefLibrary, chunkEntityMaps);
				success |= Physics::testEntitiesInVoxel(rayCoord, rayDirection, flatForward, flatRight, flatUp,
					savedVoxelCoord.voxel, chunkEntityMap, entityChunkManager, entityDefLibrary, hit);
			}

			if (success)
			{
				// The ray hit something in a voxel.
				break;
			}
		}
	}
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, double ceilingScale,
	const VoxelDouble3 &cameraForward, bool includeEntities, const VoxelChunkManager &voxelChunkManager,
	const EntityChunkManager &entityChunkManager, const CollisionChunkManager &collisionChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary, RayCastHit &hit)
{
	// Set the hit distance to max. This will ensure that if we don't hit a voxel but do hit an
	// entity, the distance can still be used.
	hit.t = RayCastHit::NO_HIT_DISTANCE;

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
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<true, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<true, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
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
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<false, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<false, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					chunkEntityMaps, hit);
			}
		}
	}

	// Return whether the ray hit something.
	return hit.t < RayCastHit::NO_HIT_DISTANCE;
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection,
	const VoxelDouble3 &cameraForward, bool includeEntities, const VoxelChunkManager &voxelChunkManager,
	const EntityChunkManager &entityChunkManager, const CollisionChunkManager &collisionChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary, RayCastHit &hit)
{
	constexpr double ceilingScale = 1.0;
	return Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraForward, includeEntities, voxelChunkManager,
		entityChunkManager, collisionChunkManager, entityDefLibrary, hit);
}

JPH::CompoundShape *Physics::getCompoundShapeFromBody(const JPH::Body &body, JPH::PhysicsSystem &physicsSystem)
{
	JPH::Shape *baseShape = const_cast<JPH::Shape*>(body.GetShape());
	DebugAssert(baseShape->GetType() == JPH::EShapeType::Compound);
	JPH::CompoundShape *derivedShape = static_cast<JPH::CompoundShape*>(baseShape);
	return derivedShape;
}

JPH::CompoundShape *Physics::getCompoundShapeFromBodyID(JPH::BodyID bodyID, JPH::PhysicsSystem &physicsSystem)
{
	if (bodyID.IsInvalid())
	{
		return nullptr;
	}

	JPH::BodyLockWrite lock(physicsSystem.GetBodyLockInterface(), bodyID);
	if (!lock.Succeeded())
	{
		return nullptr;
	}

	JPH::Body &physicsCompoundBody = lock.GetBody();
	return Physics::getCompoundShapeFromBody(physicsCompoundBody, physicsSystem);
}

JPH::StaticCompoundShape *Physics::getStaticCompoundShapeFromBody(const JPH::Body &body, JPH::PhysicsSystem &physicsSystem)
{
	JPH::CompoundShape *baseShape = Physics::getCompoundShapeFromBody(body, physicsSystem);
	DebugAssert(baseShape->GetSubType() == JPH::EShapeSubType::StaticCompound);
	JPH::StaticCompoundShape *derivedShape = static_cast<JPH::StaticCompoundShape*>(baseShape);
	return derivedShape;
}

JPH::StaticCompoundShape *Physics::getStaticCompoundShapeFromBodyID(JPH::BodyID bodyID, JPH::PhysicsSystem &physicsSystem)
{
	JPH::CompoundShape *baseShape = Physics::getCompoundShapeFromBodyID(bodyID, physicsSystem);
	DebugAssert(baseShape != nullptr);
	DebugAssert(baseShape->GetSubType() == JPH::EShapeSubType::StaticCompound);
	JPH::StaticCompoundShape *derivedShape = static_cast<JPH::StaticCompoundShape*>(baseShape);
	return derivedShape;
}
