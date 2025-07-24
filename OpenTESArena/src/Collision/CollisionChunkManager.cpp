#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyLock.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"

#include "CollisionChunkManager.h"
#include "CollisionShapeDefinition.h"
#include "Physics.h"
#include "PhysicsLayer.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelBoxCombineChunkManager.h"
#include "../World/MeshUtils.h"

namespace
{
	enum class CompoundShapeCategory
	{
		Walls,
		Doors,
		Sensors
	};

	void MakePhysicsColliderInitValues(SNInt xMin, int yMin, WEInt zMin, SNInt xMax, int yMax, WEInt zMax, const ChunkInt2 &chunkPos,
		const CollisionShapeDefinition &collisionShapeDef, VoxelShapeScaleType scaleType, double ceilingScale, bool isSensor, JPH::PhysicsSystem &physicsSystem,
		JPH::BoxShapeSettings *outBoxShapeSettings, JPH::Vec3 *outPosition, JPH::Quat *outRotation)
	{
		const SNInt boxVoxelWidth = (xMax - xMin) + 1;
		const int boxVoxelHeight = (yMax - yMin) + 1;
		const WEInt boxVoxelDepth = (zMax - zMin) + 1;
		const SNDouble boxVoxelWidthReal = static_cast<SNDouble>(boxVoxelWidth);
		const double boxVoxelHeightReal = static_cast<double>(boxVoxelHeight);
		const WEDouble boxVoxelDepthReal = static_cast<WEDouble>(boxVoxelDepth);

		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &boxShapeDef = collisionShapeDef.box;
		const double yMinMeshBottom = (static_cast<double>(yMin) * ceilingScale) + MeshUtils::getScaledVertexY(boxShapeDef.yOffset, scaleType, ceilingScale);
		const double yMaxMeshTop = (static_cast<double>(yMax) * ceilingScale) + MeshUtils::getScaledVertexY(boxShapeDef.yOffset + boxShapeDef.height, scaleType, ceilingScale);
		const double scaledHeight = yMaxMeshTop - yMinMeshBottom;
		const double scaledHalfHeight = scaledHeight * 0.50;

		outBoxShapeSettings->mHalfExtent = JPH::Vec3(
			static_cast<float>((boxShapeDef.width * boxVoxelWidthReal) * 0.50),
			static_cast<float>(scaledHalfHeight),
			static_cast<float>((boxShapeDef.depth * boxVoxelDepthReal) * 0.50));
		outBoxShapeSettings->mConvexRadius = static_cast<float>(Physics::BoxConvexRadius);

		const WorldInt3 boxWorldVoxelPosMin = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt3(xMin, yMin, zMin));
		const WorldDouble3 boxWorldPointMin(
			static_cast<double>(boxWorldVoxelPosMin.x),
			static_cast<double>(boxWorldVoxelPosMin.y),
			static_cast<double>(boxWorldVoxelPosMin.z));
		*outPosition = JPH::Vec3(
			static_cast<float>(boxWorldPointMin.x + (boxVoxelWidthReal * 0.50)),
			static_cast<float>(yMinMeshBottom + scaledHalfHeight),
			static_cast<float>(boxWorldPointMin.z + (boxVoxelDepthReal * 0.50)));

		const RadiansF boxYRotation = static_cast<RadiansF>(boxShapeDef.yRotation);
		*outRotation = JPH::Quat::sRotation(JPH::Vec3Arg::sAxisY(), boxYRotation);
	}

	JPH::BodyID CreateChunkCompoundShape(const CollisionChunk &collisionChunk, double ceilingScale, CompoundShapeCategory category,
		const VoxelChunk &voxelChunk, const VoxelBoxCombineChunk &boxCombineChunk, JPH::PhysicsSystem &physicsSystem)
	{
		const ChunkInt2 chunkPos = collisionChunk.position;
		const int chunkHeight = collisionChunk.height;

		JPH::StaticCompoundShapeSettings compoundShapeSettings;
		compoundShapeSettings.SetEmbedded();

		std::vector<JPH::Ref<JPH::BoxShapeSettings>> boxShapeSettingsList; // Freed at end of scope

		const RecyclablePool<VoxelBoxCombineResultID, VoxelBoxCombineResult> &combinedBoxesPool = boxCombineChunk.combinedBoxesPool;
		for (int i = 0; i < combinedBoxesPool.getTotalCount(); i++)
		{
			const VoxelBoxCombineResult *combinedBoxResult = combinedBoxesPool.tryGet(i);
			if (combinedBoxResult == nullptr)
			{
				continue;
			}

			const VoxelInt3 combinedBoxMin = combinedBoxResult->min;
			const VoxelInt3 combinedBoxMax = combinedBoxResult->max;

			bool isTriggerVoxel = false;
			VoxelTriggerDefID triggerDefID;
			if (voxelChunk.tryGetTriggerDefID(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z, &triggerDefID))
			{
				const VoxelTriggerDefinition &voxelTriggerDef = voxelChunk.triggerDefs[triggerDefID];
				isTriggerVoxel = voxelTriggerDef.hasValidDefForPhysics();
			}

			bool isInteriorLevelChangeVoxel = false;
			VoxelTransitionDefID transitionDefID;
			if (voxelChunk.tryGetTransitionDefID(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z, &transitionDefID))
			{
				const TransitionDefinition &transitionDef = voxelChunk.transitionDefs[transitionDefID];
				isInteriorLevelChangeVoxel = transitionDef.type == TransitionType::InteriorLevelChange;
			}

			bool isDoorVoxel = false;
			bool isClosedDoor = false;
			VoxelDoorDefID doorDefID;
			if (voxelChunk.tryGetDoorDefID(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z, &doorDefID))
			{
				isDoorVoxel = true;
				int doorAnimInstIndex;
				if (voxelChunk.tryGetDoorAnimInstIndex(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z, &doorAnimInstIndex))
				{
					const VoxelDoorAnimationInstance &doorAnimInst = voxelChunk.doorAnimInsts[doorAnimInstIndex];
					isClosedDoor = doorAnimInst.stateType == VoxelDoorAnimationStateType::Closed;
				}
				else
				{
					isClosedDoor = true;
				}
			}

			const bool voxelHasCollision = collisionChunk.enabledColliders.get(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z);
			const bool isSensorCollider = isTriggerVoxel || isInteriorLevelChangeVoxel;

			const bool shouldAddWall = (voxelHasCollision && !isSensorCollider && !isDoorVoxel) && (category == CompoundShapeCategory::Walls);
			const bool shouldAddDoor = isClosedDoor && (category == CompoundShapeCategory::Doors);
			const bool shouldAddSensor = isSensorCollider && (category == CompoundShapeCategory::Sensors);
			const bool shouldCreateCollider = shouldAddWall || shouldAddDoor || shouldAddSensor;
			if (!shouldCreateCollider)
			{
				continue;
			}

			const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z);
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.shapeDefs[voxelShapeDefID];
			const CollisionShapeDefID collisionShapeDefID = collisionChunk.shapeDefIDs.get(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z);
			const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

			boxShapeSettingsList.emplace_back(new JPH::BoxShapeSettings());
			JPH::BoxShapeSettings *boxShapeSettings = boxShapeSettingsList.back().GetPtr();

			JPH::Vec3 boxPosition;
			JPH::Quat boxRotation;
			MakePhysicsColliderInitValues(combinedBoxMin.x, combinedBoxMin.y, combinedBoxMin.z, combinedBoxMax.x, combinedBoxMax.y, combinedBoxMax.z,
				chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, boxShapeSettings, &boxPosition, &boxRotation);

			compoundShapeSettings.AddShape(boxPosition, boxRotation, boxShapeSettings);
		}

		if (compoundShapeSettings.mSubShapes.empty())
		{
			// Jolt doesn't like creating a compound shape with 0 sub-shapes
			return Physics::INVALID_BODY_ID;
		}

		const JPH::Vec3 compoundBodyPosition = JPH::Vec3::sZero();
		const JPH::Quat compoundBodyRotation = JPH::Quat::sIdentity();
		const JPH::ObjectLayer objectLayer = (category == CompoundShapeCategory::Sensors) ? PhysicsLayers::SENSOR : PhysicsLayers::NON_MOVING;
		JPH::BodyCreationSettings compoundBodyCreationSettings(&compoundShapeSettings, compoundBodyPosition, compoundBodyRotation, JPH::EMotionType::Static, objectLayer);

		if (category == CompoundShapeCategory::Walls)
		{
			// Keep player from erratically hopping/skipping as much when running due to no contact welding in Jolt.
			compoundBodyCreationSettings.mEnhancedInternalEdgeRemoval = true;
		}
		else if (category == CompoundShapeCategory::Sensors)
		{
			compoundBodyCreationSettings.mIsSensor = true;
		}

		JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
		JPH::BodyID compoundBodyID = bodyInterface.CreateAndAddBody(compoundBodyCreationSettings, JPH::EActivation::Activate);
		return compoundBodyID;
	}
}

void CollisionChunkManager::populateChunkShapeDefs(CollisionChunk &collisionChunk, const VoxelChunk &voxelChunk)
{
	const int chunkHeight = collisionChunk.height;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(x, y, z);
				CollisionShapeDefID collisionShapeDefID = collisionChunk.findShapeDefIdMapping(voxelChunk, voxelShapeDefID);
				if (collisionShapeDefID == -1)
				{
					collisionShapeDefID = collisionChunk.addShapeDefIdMapping(voxelChunk, voxelShapeDefID);
				}

				collisionChunk.shapeDefIDs.set(x, y, z, collisionShapeDefID);
			}
		}
	}
}

void CollisionChunkManager::populateChunkEnabledColliders(CollisionChunk &collisionChunk, const VoxelChunk &voxelChunk)
{
	const int chunkHeight = collisionChunk.height;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				bool voxelHasCollision = false;

				int doorAnimInstIndex;
				if (voxelChunk.tryGetDoorAnimInstIndex(x, y, z, &doorAnimInstIndex))
				{
					const VoxelDoorAnimationInstance &doorAnimInst = voxelChunk.doorAnimInsts[doorAnimInstIndex];
					voxelHasCollision = doorAnimInst.stateType == VoxelDoorAnimationStateType::Closed;
				}
				else
				{
					const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(x, y, z);
					const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];
					voxelHasCollision = voxelTraitsDef.hasCollision();
				}

				collisionChunk.enabledColliders.set(x, y, z, voxelHasCollision);
			}
		}
	}
}

void CollisionChunkManager::populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk,
	const VoxelBoxCombineChunk &boxCombineChunk, JPH::PhysicsSystem &physicsSystem)
{
	const int chunkHeight = voxelChunk.height;
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	this->populateChunkShapeDefs(collisionChunk, voxelChunk); // @todo: give positions span that is all WxHxD
	this->populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give positions span that is all WxHxD

	DebugAssert(collisionChunk.wallCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(collisionChunk.doorCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(collisionChunk.sensorCompoundBodyID == Physics::INVALID_BODY_ID);
	collisionChunk.wallCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Walls, voxelChunk, boxCombineChunk, physicsSystem);
	collisionChunk.doorCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Doors, voxelChunk, boxCombineChunk, physicsSystem);
	collisionChunk.sensorCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Sensors, voxelChunk, boxCombineChunk, physicsSystem);
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk,
	const VoxelBoxCombineChunk &boxCombineChunk, JPH::PhysicsSystem &physicsSystem)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	// @todo: this dirty shapes list might be full of brand new voxels this frame, so we're accidentally destroying + recreating them all (found during the AddBodiesPrepare/Finalize() work)

	const Span<const VoxelInt3> dirtyShapeDefPositions = voxelChunk.dirtyShapeDefPositions;
	const Span<const VoxelInt3> dirtyDoorAnimInstPositions = voxelChunk.dirtyDoorAnimInstPositions;

	if (dirtyShapeDefPositions.getCount() > 0)
	{
		if (collisionChunk.wallCompoundBodyID != Physics::INVALID_BODY_ID)
		{
			bodyInterface.RemoveBody(collisionChunk.wallCompoundBodyID);
			bodyInterface.DestroyBody(collisionChunk.wallCompoundBodyID);
			collisionChunk.wallCompoundBodyID = Physics::INVALID_BODY_ID;
		}

		if (collisionChunk.sensorCompoundBodyID != Physics::INVALID_BODY_ID)
		{
			bodyInterface.RemoveBody(collisionChunk.sensorCompoundBodyID);
			bodyInterface.DestroyBody(collisionChunk.sensorCompoundBodyID);
			collisionChunk.sensorCompoundBodyID = Physics::INVALID_BODY_ID;
		}

		this->populateChunkShapeDefs(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster
		this->populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster

		collisionChunk.wallCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Walls, voxelChunk, boxCombineChunk, physicsSystem);
		collisionChunk.sensorCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Sensors, voxelChunk, boxCombineChunk, physicsSystem);
	}

	if (dirtyDoorAnimInstPositions.getCount() > 0)
	{
		if (collisionChunk.doorCompoundBodyID != Physics::INVALID_BODY_ID)
		{
			bodyInterface.RemoveBody(collisionChunk.doorCompoundBodyID);
			bodyInterface.DestroyBody(collisionChunk.doorCompoundBodyID);
			collisionChunk.doorCompoundBodyID = Physics::INVALID_BODY_ID;
		}

		this->populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster

		collisionChunk.doorCompoundBodyID = CreateChunkCompoundShape(collisionChunk, ceilingScale, CompoundShapeCategory::Doors, voxelChunk, boxCombineChunk, physicsSystem);
	}
}

void CollisionChunkManager::update(double dt, Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	Span<const ChunkInt2> freedChunkPositions, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
	const VoxelBoxCombineChunkManager &voxelBoxCombineChunkManager, JPH::PhysicsSystem &physicsSystem)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		CollisionChunk &collisionChunk = this->getChunkAtIndex(chunkIndex);
		collisionChunk.freePhysicsCompoundBodies(bodyInterface);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const int spawnIndex = this->spawnChunk();
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelBoxCombineChunk &boxCombineChunk = voxelBoxCombineChunkManager.getChunkAtPosition(chunkPos);
		this->populateChunk(spawnIndex, ceilingScale, chunkPos, voxelChunk, boxCombineChunk, physicsSystem);
	}

	// Update dirty voxels.
	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelBoxCombineChunk &boxCombineChunk = voxelBoxCombineChunkManager.getChunkAtPosition(chunkPos);
		this->updateDirtyVoxels(chunkPos, ceilingScale, voxelChunk, boxCombineChunk, physicsSystem);
	}

	this->chunkPool.clear();
}

void CollisionChunkManager::clear(JPH::PhysicsSystem &physicsSystem)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freePhysicsCompoundBodies(bodyInterface);
		this->recycleChunk(i);
	}
}
