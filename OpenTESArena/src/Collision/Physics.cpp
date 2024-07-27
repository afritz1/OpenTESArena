#include <algorithm>
#include <cmath>

#include "Physics.h"
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
#include "../Voxels/VoxelFacing3D.h"
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
					const EntityChunk *entityChunkPtr = entityChunkManager.tryGetChunkAtPosition(ChunkInt2(x, z));
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
			const EntityChunk *entityChunkPtr = entityChunkManager.tryGetChunkAtPosition(ChunkInt2(chunkX, chunkZ));
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

		ChunkEntityMap chunkEntityMap;
		chunkEntityMap.init(chunk);

		// Build mappings of voxels to entities.
		for (const EntityInstanceID entityInstID : entityInstIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const CoordDouble2 viewCoordXZ(viewCoord.chunk, VoxelDouble2(viewCoord.point.x, viewCoord.point.z));
			
			EntityObservedResult observedResult;
			entityChunkManager.getEntityObservedResult(entityInstID, viewCoordXZ, observedResult);

			const CoordDouble3 entityCoord = entityChunkManager.getEntityPosition3D(entityInstID, ceilingScale, voxelChunkManager);

			// Get the entity's view-independent bounding box to help determine which voxels they are in.
			const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
			const CoordDouble3 minCoord = ChunkUtils::recalculateCoord(entityCoord.chunk, entityCoord.point - Double3(entityBBox.halfWidth, 0.0, entityBBox.halfDepth));
			const CoordDouble3 maxCoord = ChunkUtils::recalculateCoord(entityCoord.chunk, entityCoord.point + Double3(entityBBox.halfWidth, entityBBox.height, entityBBox.halfDepth));

			// Get min and max coordinates in chunk space and get the difference for iteration.
			const CoordInt3 minVoxelCoord(minCoord.chunk, VoxelUtils::pointToVoxel(minCoord.point, ceilingScale));
			const CoordInt3 maxVoxelCoord(maxCoord.chunk, VoxelUtils::pointToVoxel(maxCoord.point, ceilingScale));
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
		const CoordDouble3 &rayPoint, const VoxelDouble3 &rayDirection, CoordDouble3 *outHitPoint)
	{
		// Do a ray test to see if the ray intersects.
		const WorldDouble3 absoluteRayPoint = VoxelUtils::coordToWorldPoint(rayPoint);
		const WorldDouble3 absoluteFlatPosition = VoxelUtils::coordToWorldPoint(entityCoord);
		WorldDouble3 absoluteHitPoint;
		if (MathUtils::rayPlaneIntersection(absoluteRayPoint, rayDirection, absoluteFlatPosition,
			entityForward, &absoluteHitPoint))
		{
			const WorldDouble3 diff = absoluteHitPoint - absoluteFlatPosition;

			// Get the texture coordinates. It's okay if they are outside the entity.
			const Double2 uv(
				0.5 - (diff.dot(entityRight) / entityWidth),
				1.0 - (diff.dot(entityUp) / entityHeight));

			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;
			DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
			const EntityAnimationDefinitionKeyframe &animKeyframe = animDef.keyframes[linearizedKeyframeIndex];
			const TextureAsset &textureAsset = animKeyframe.textureAsset;

			// The entity's projected rectangle is hit if the texture coordinates are valid.
			const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);

			*outHitPoint = VoxelUtils::worldPointToCoord(absoluteHitPoint);
			return withinEntity;
		}
		else
		{
			// Did not intersect the entity's plane.
			return false;
		}
	}

	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	bool testInitialVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const VoxelInt3 &voxel,
		VoxelFacing3D farFacing, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const CollisionChunkManager &collisionChunkManager, Physics::Hit &hit)
	{
		const VoxelChunk *voxelChunk = voxelChunkManager.tryGetChunkAtPosition(rayCoord.chunk);
		if (voxelChunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		const CollisionChunk *collisionChunk = collisionChunkManager.tryGetChunkAtPosition(rayCoord.chunk);
		DebugAssert(collisionChunk != nullptr);

		if (!voxelChunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		if (!collisionChunk->enabledColliders.get(voxel.x, voxel.y, voxel.z))
		{
			// Collider is not turned on.
			return false;
		}

		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk->getMeshDefID(voxel.x, voxel.y, voxel.z);
		const VoxelMeshDefinition &voxelMeshDef = voxelChunk->getMeshDef(voxelMeshDefID);
		const VoxelMeshScaleType voxelMeshScaleType = voxelMeshDef.scaleType;

		const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk->meshDefIDs.get(voxel.x, voxel.y, voxel.z);
		const CollisionMeshDefinition &collisionMeshDef = collisionChunk->getCollisionMeshDef(collisionMeshDefID);
		const BufferView<const double> verticesView(collisionMeshDef.vertices);
		const BufferView<const double> normalsView(collisionMeshDef.normals);
		const BufferView<const int> indicesView(collisionMeshDef.indices);

		// Each collision triangle is formed by 6 indices.
		static_assert(CollisionMeshDefinition::INDICES_PER_TRIANGLE == 6);

		const VoxelDouble3 voxelReal(
			static_cast<SNDouble>(voxel.x),
			static_cast<double>(voxel.y) * ceilingScale,
			static_cast<WEDouble>(voxel.z));
		bool success = false;
		for (int i = 0; i < collisionMeshDef.triangleCount; i++)
		{
			const int indicesIndex = i * CollisionMeshDefinition::INDICES_PER_TRIANGLE;
			const int vertexIndex0 = indicesView.get(indicesIndex) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int vertexIndex1 = indicesView.get(indicesIndex + 2) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int vertexIndex2 = indicesView.get(indicesIndex + 4) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			//const int normalIndex0 = indicesView.get(indicesIndex + 1) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			//const int normalIndex1 = indicesView.get(indicesIndex + 3) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			//const int normalIndex2 = indicesView.get(indicesIndex + 5) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;

			const double vertex0X = verticesView.get(vertexIndex0);
			const double vertex0Y = verticesView.get(vertexIndex0 + 1);
			const double vertex0Z = verticesView.get(vertexIndex0 + 2);
			const double vertex1X = verticesView.get(vertexIndex1);
			const double vertex1Y = verticesView.get(vertexIndex1 + 1);
			const double vertex1Z = verticesView.get(vertexIndex1 + 2);
			const double vertex2X = verticesView.get(vertexIndex2);
			const double vertex2Y = verticesView.get(vertexIndex2 + 1);
			const double vertex2Z = verticesView.get(vertexIndex2 + 2);

			// Normals are the same per face.
			//const double normal0X = normalsView.get(normalIndex0);
			//const double normal0Y = normalsView.get(normalIndex0 + 1);
			//const double normal0Z = normalsView.get(normalIndex0 + 2);

			const Double3 v0(vertex0X, vertex0Y, vertex0Z);
			const Double3 v1(vertex1X, vertex1Y, vertex1Z);
			const Double3 v2(vertex2X, vertex2Y, vertex2Z);

			const Double3 v0Actual(
				v0.x + voxelReal.x,
				MeshUtils::getScaledVertexY(v0.y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				v0.z + voxelReal.z);
			const Double3 v1Actual(
				v1.x + voxelReal.x,
				MeshUtils::getScaledVertexY(v1.y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				v1.z + voxelReal.z);
			const Double3 v2Actual(
				v2.x + voxelReal.x,
				MeshUtils::getScaledVertexY(v2.y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				v2.z + voxelReal.z);

			double t;
			success = MathUtils::rayTriangleIntersection(rayCoord.point, rayDirection, v0Actual, v1Actual, v2Actual, &t);
			if (success)
			{
				const CoordDouble3 hitCoord(rayCoord.chunk, rayCoord.point + (rayDirection * t));
				hit.initVoxel(t, hitCoord, voxel, &farFacing);
				break;
			}
		}

		return success;
	}

	// Checks a voxel for ray hits and writes them into the output parameter. The near point and far point
	// are in the voxel coord's chunk, not necessarily the ray's. Returns true if the ray hit something.
	bool testVoxelRay(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const CoordInt3 &voxelCoord,
		VoxelFacing3D nearFacing, const CoordDouble3 &nearCoord, const CoordDouble3 &farCoord,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, const CollisionChunkManager &collisionChunkManager,
		Physics::Hit &hit)
	{
		const VoxelChunk *voxelChunk = voxelChunkManager.tryGetChunkAtPosition(voxelCoord.chunk);
		if (voxelChunk == nullptr)
		{
			// Nothing to intersect with.
			return false;
		}

		const CollisionChunk *collisionChunk = collisionChunkManager.tryGetChunkAtPosition(voxelCoord.chunk);
		DebugAssert(collisionChunk != nullptr);

		const VoxelInt3 &voxel = voxelCoord.voxel;
		if (!collisionChunk->isValidVoxel(voxel.x, voxel.y, voxel.z))
		{
			// Not in the chunk.
			return false;
		}

		if (!collisionChunk->enabledColliders.get(voxel.x, voxel.y, voxel.z))
		{
			// Collider is not turned on.
			return false;
		}

		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk->getMeshDefID(voxel.x, voxel.y, voxel.z);
		const VoxelMeshDefinition &voxelMeshDef = voxelChunk->getMeshDef(voxelMeshDefID);
		const VoxelMeshScaleType voxelMeshScaleType = voxelMeshDef.scaleType;

		const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk->meshDefIDs.get(voxel.x, voxel.y, voxel.z);
		const CollisionMeshDefinition &collisionMeshDef = collisionChunk->getCollisionMeshDef(collisionMeshDefID);
		const BufferView<const double> verticesView(collisionMeshDef.vertices);
		const BufferView<const double> normalsView(collisionMeshDef.normals);
		const BufferView<const int> indicesView(collisionMeshDef.indices);

		// Each collision triangle is formed by 6 indices.
		static_assert(CollisionMeshDefinition::INDICES_PER_TRIANGLE == 6);

		const VoxelDouble3 voxelReal(
			static_cast<SNDouble>(voxel.x),
			static_cast<double>(voxel.y) * ceilingScale,
			static_cast<WEDouble>(voxel.z));
		const WorldDouble3 rayStartWorldPoint = VoxelUtils::coordToWorldPoint(rayCoord);

		bool success = false;
		for (int i = 0; i < collisionMeshDef.triangleCount; i++)
		{
			const int indicesIndex = i * CollisionMeshDefinition::INDICES_PER_TRIANGLE;
			const int vertexIndex0 = indicesView.get(indicesIndex) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int vertexIndex1 = indicesView.get(indicesIndex + 2) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int vertexIndex2 = indicesView.get(indicesIndex + 4) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			//const int normalIndex0 = indicesView.get(indicesIndex + 1) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			//const int normalIndex1 = indicesView.get(indicesIndex + 3) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			//const int normalIndex2 = indicesView.get(indicesIndex + 5) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;

			const double vertex0X = verticesView.get(vertexIndex0);
			const double vertex0Y = verticesView.get(vertexIndex0 + 1);
			const double vertex0Z = verticesView.get(vertexIndex0 + 2);
			const double vertex1X = verticesView.get(vertexIndex1);
			const double vertex1Y = verticesView.get(vertexIndex1 + 1);
			const double vertex1Z = verticesView.get(vertexIndex1 + 2);
			const double vertex2X = verticesView.get(vertexIndex2);
			const double vertex2Y = verticesView.get(vertexIndex2 + 1);
			const double vertex2Z = verticesView.get(vertexIndex2 + 2);

			// Normals are the same per face.
			//const double normal0X = normalsView.get(normalIndex0);
			//const double normal0Y = normalsView.get(normalIndex0 + 1);
			//const double normal0Z = normalsView.get(normalIndex0 + 2);

			const Double3 v0Actual(
				vertex0X + voxelReal.x,
				MeshUtils::getScaledVertexY(vertex0Y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				vertex0Z + voxelReal.z);
			const Double3 v1Actual(
				vertex1X + voxelReal.x,
				MeshUtils::getScaledVertexY(vertex1Y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				vertex1Z + voxelReal.z);
			const Double3 v2Actual(
				vertex2X + voxelReal.x,
				MeshUtils::getScaledVertexY(vertex2Y, voxelMeshScaleType, ceilingScale) + voxelReal.y,
				vertex2Z + voxelReal.z);

			const WorldDouble3 v0World = VoxelUtils::chunkPointToWorldPoint(voxelCoord.chunk, v0Actual);
			const WorldDouble3 v1World = VoxelUtils::chunkPointToWorldPoint(voxelCoord.chunk, v1Actual);
			const WorldDouble3 v2World = VoxelUtils::chunkPointToWorldPoint(voxelCoord.chunk, v2Actual);

			double t;
			success = MathUtils::rayTriangleIntersection(rayStartWorldPoint, rayDirection, v0World, v1World, v2World, &t);
			if (success)
			{
				const CoordDouble3 hitCoord = ChunkUtils::recalculateCoord(rayCoord.chunk, rayCoord.point + (rayDirection * t));
				const VoxelFacing3D facing = nearFacing; // @todo: probably needs to take hit normal into account
				hit.initVoxel(t, hitCoord, voxel, &facing);
				break;
			}
		}

		return success;
	}

	// Helper function for testing which entities in a voxel are intersected by a ray.
	bool testEntitiesInVoxel(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection,
		const VoxelDouble3 &flatForward, const VoxelDouble3 &flatRight, const VoxelDouble3 &flatUp,
		const VoxelInt3 &voxel, const ChunkEntityMap &chunkEntityMap, const EntityChunkManager &entityChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer, Physics::Hit &hit)
	{
		// Use a separate hit variable so we can determine whether an entity was closer.
		Physics::Hit entityHit;
		entityHit.setT(Hit::MAX_T);

		const auto &entityMappings = chunkEntityMap.mappings;
		const auto iter = entityMappings.find(voxel);
		if (iter != entityMappings.end())
		{
			// Iterate over all entities that cross this voxel and ray test them.
			BufferView<const EntityEntry> entityEntryList = iter->second;
			for (const EntityEntry &entry : entityEntryList)
			{
				const EntityObservedResult &observedResult = entry.observedResult;
				const EntityInstanceID entityInstID = observedResult.entityInstID;
				const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;

				const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
				const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
				const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
				DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
				const EntityAnimationDefinitionKeyframe &animKeyframe = animDef.keyframes[linearizedKeyframeIndex];
				const double flatWidth = animKeyframe.width;
				const double flatHeight = animKeyframe.height;

				CoordDouble3 hitCoord;
				if (Physics::getEntityRayIntersection(observedResult, entry.coord, entityDef, flatForward, flatRight, flatUp,
					flatWidth, flatHeight, rayCoord, rayDirection, &hitCoord))
				{
					const double distance = (hitCoord - rayCoord).length();
					if (distance < entityHit.getT())
					{
						entityHit.initEntity(distance, hitCoord, entityInstID);
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
	void rayCastInternal(const CoordDouble3 &rayCoord, const VoxelDouble3 &rayDirection, const VoxelDouble3 &cameraForward,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
		const CollisionChunkManager &collisionChunkManager, bool includeEntities, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, std::vector<ChunkEntityMap> &chunkEntityMaps, Physics::Hit &hit)
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
		const VoxelChunk *currentChunkPtr = voxelChunkManager.tryGetChunkAtPosition(currentChunk);

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
					rayVoxel, chunkEntityMap, entityChunkManager, entityDefLibrary, renderer, hit);
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
				currentChunkPtr = voxelChunkManager.tryGetChunkAtPosition(currentChunk);
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
				nearCoord, farCoord, ceilingScale, voxelChunkManager, collisionChunkManager, hit);

			if (includeEntities)
			{
				// Test the current voxel's entities for ray intersections.
				const ChunkEntityMap &chunkEntityMap = Physics::getOrAddChunkEntityMap(savedVoxelCoord.chunk, rayCoord,
					ceilingScale, voxelChunkManager, entityChunkManager, entityDefLibrary, chunkEntityMaps);
				success |= Physics::testEntitiesInVoxel(rayCoord, rayDirection, flatForward, flatRight, flatUp,
					savedVoxelCoord.voxel, chunkEntityMap, entityChunkManager, entityDefLibrary, renderer, hit);
			}

			if (success)
			{
				// The ray hit something in a voxel.
				break;
			}
		}
	}
}

void Physics::Hit::initVoxel(double t, const CoordDouble3 &coord, const VoxelInt3 &voxel, const VoxelFacing3D *facing)
{
	this->t = t;
	this->coord = coord;
	this->type = HitType::Voxel;
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

void Physics::Hit::initEntity(double t, const CoordDouble3 &coord, EntityInstanceID id)
{
	this->t = t;
	this->coord = coord;
	this->type = HitType::Entity;
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

const CoordDouble3 &Physics::Hit::getCoord() const
{
	return this->coord;
}

Physics::HitType Physics::Hit::getType() const
{
	return this->type;
}

const Physics::Hit::VoxelHit &Physics::Hit::getVoxelHit() const
{
	DebugAssert(this->getType() == HitType::Voxel);
	return this->voxelHit;
}

const Physics::Hit::EntityHit &Physics::Hit::getEntityHit() const
{
	DebugAssert(this->getType() == HitType::Entity);
	return this->entityHit;
}

void Physics::Hit::setT(double t)
{
	this->t = t;
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, double ceilingScale,
	const VoxelDouble3 &cameraForward, bool includeEntities, const VoxelChunkManager &voxelChunkManager,
	const EntityChunkManager &entityChunkManager, const CollisionChunkManager &collisionChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer, Physics::Hit &hit)
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
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<true, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<true, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<true, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
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
					renderer, chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<false, true, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				Physics::rayCastInternal<false, false, true>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
			else
			{
				Physics::rayCastInternal<false, false, false>(rayStart, rayDirection, cameraForward, ceilingScale,
					voxelChunkManager, entityChunkManager, collisionChunkManager, includeEntities, entityDefLibrary,
					renderer, chunkEntityMaps, hit);
			}
		}
	}

	// Return whether the ray hit something.
	return hit.getT() < Hit::MAX_T;
}

bool Physics::rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection,
	const VoxelDouble3 &cameraForward, bool includeEntities, const VoxelChunkManager &voxelChunkManager,
	const EntityChunkManager &entityChunkManager, const CollisionChunkManager &collisionChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer, Physics::Hit &hit)
{
	constexpr double ceilingScale = 1.0;
	return Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraForward, includeEntities, voxelChunkManager,
		entityChunkManager, collisionChunkManager, entityDefLibrary, renderer, hit);
}
