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
#include "../World/MeshUtils.h"

namespace
{
	enum class CompoundShapeCategory
	{
		Walls,
		Doors,
		Sensors
	};

	void MakePhysicsColliderInitValues(SNInt x, int y, WEInt z, const ChunkInt2 &chunkPos, const CollisionShapeDefinition &collisionShapeDef,
		VoxelShapeScaleType scaleType, double ceilingScale, bool isSensor, JPH::PhysicsSystem &physicsSystem,
		JPH::BoxShapeSettings *outBoxShapeSettings, JPH::Vec3 *outPosition, JPH::Quat *outRotation)
	{
		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &boxShapeDef = collisionShapeDef.box;
		const double voxelYBottom = static_cast<double>(y) * ceilingScale;
		const double scaledYBottom = voxelYBottom + MeshUtils::getScaledVertexY(boxShapeDef.yOffset, scaleType, ceilingScale);
		const double scaledYTop = voxelYBottom + MeshUtils::getScaledVertexY(boxShapeDef.yOffset + boxShapeDef.height, scaleType, ceilingScale);
		const double scaledHeight = scaledYTop - scaledYBottom;
		const double scaledHalfHeight = scaledHeight * 0.50;

		outBoxShapeSettings->mHalfExtent = JPH::Vec3(
			static_cast<float>(boxShapeDef.width * 0.50),
			static_cast<float>(scaledHalfHeight),
			static_cast<float>(boxShapeDef.depth * 0.50));
		outBoxShapeSettings->mConvexRadius = static_cast<float>(Physics::BoxConvexRadius);

		const WorldInt3 boxWorldVoxelPos = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt3(x, y, z));
		*outPosition = JPH::Vec3(
			static_cast<float>(boxWorldVoxelPos.x + 0.50),
			static_cast<float>(scaledYBottom + scaledHalfHeight),
			static_cast<float>(boxWorldVoxelPos.z + 0.50));

		const RadiansF boxYRotation = static_cast<RadiansF>(boxShapeDef.yRotation);
		*outRotation = JPH::Quat::sRotation(JPH::Vec3Arg::sAxisY(), boxYRotation);
	}

	JPH::BodyID createChunkCompoundShape(const CollisionChunk &collisionChunk, const VoxelChunk &voxelChunk, double ceilingScale, CompoundShapeCategory category, JPH::PhysicsSystem &physicsSystem)
	{
		const ChunkInt2 chunkPos = collisionChunk.getPosition();
		const int chunkHeight = collisionChunk.getHeight();

		JPH::StaticCompoundShapeSettings compoundShapeSettings;
		compoundShapeSettings.SetEmbedded();

		std::vector<JPH::Ref<JPH::BoxShapeSettings>> boxShapeSettingsList; // Freed at end of scope

		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (int y = 0; y < chunkHeight; y++)
			{
				for (SNInt x = 0; x < Chunk::WIDTH; x++)
				{
					bool isTriggerVoxel = false;
					VoxelTriggerDefID triggerDefID;
					if (voxelChunk.tryGetTriggerDefID(x, y, z, &triggerDefID))
					{
						const VoxelTriggerDefinition &voxelTriggerDef = voxelChunk.getTriggerDef(triggerDefID);
						isTriggerVoxel = voxelTriggerDef.hasValidDefForPhysics();
					}

					bool isInteriorLevelChangeVoxel = false;
					VoxelTransitionDefID transitionDefID;
					if (voxelChunk.tryGetTransitionDefID(x, y, z, &transitionDefID))
					{
						const TransitionDefinition &transitionDef = voxelChunk.getTransitionDef(transitionDefID);
						isInteriorLevelChangeVoxel = transitionDef.type == TransitionType::InteriorLevelChange;
					}

					bool isDoorVoxel = false;
					bool isClosedDoor = false;
					VoxelDoorDefID doorDefID;
					if (voxelChunk.tryGetDoorDefID(x, y, z, &doorDefID))
					{
						isDoorVoxel = true;
						int doorAnimInstIndex;
						if (voxelChunk.tryGetDoorAnimInstIndex(x, y, z, &doorAnimInstIndex))
						{
							const BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
							const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
							isClosedDoor = doorAnimInst.stateType == VoxelDoorAnimationInstance::StateType::Closed;
						}
						else
						{
							isClosedDoor = true;
						}
					}

					const bool voxelHasCollision = collisionChunk.enabledColliders.get(x, y, z);
					const bool isSensorCollider = isTriggerVoxel || isInteriorLevelChangeVoxel;

					const bool shouldAddWall = (voxelHasCollision && !isSensorCollider && !isDoorVoxel) && (category == CompoundShapeCategory::Walls);
					const bool shouldAddDoor = isClosedDoor && (category == CompoundShapeCategory::Doors);
					const bool shouldAddSensor = isSensorCollider && (category == CompoundShapeCategory::Sensors);
					const bool shouldCreateCollider = shouldAddWall || shouldAddDoor || shouldAddSensor;

					if (shouldCreateCollider)
					{
						const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(x, y, z);
						const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
						const CollisionShapeDefID collisionShapeDefID = collisionChunk.shapeDefIDs.get(x, y, z);
						const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

						boxShapeSettingsList.emplace_back(new JPH::BoxShapeSettings());
						JPH::BoxShapeSettings *boxShapeSettings = boxShapeSettingsList.back().GetPtr();

						JPH::Vec3 boxPosition;
						JPH::Quat boxRotation;
						MakePhysicsColliderInitValues(x, y, z, chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, boxShapeSettings, &boxPosition, &boxRotation);

						compoundShapeSettings.AddShape(boxPosition, boxRotation, boxShapeSettings);
					}
				}
			}
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
	const int chunkHeight = collisionChunk.getHeight();

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(x, y, z);
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
	const int chunkHeight = collisionChunk.getHeight();

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
					const BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
					const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
					voxelHasCollision = doorAnimInst.stateType == VoxelDoorAnimationInstance::StateType::Closed;
				}
				else
				{
					const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(x, y, z);
					const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
					voxelHasCollision = voxelTraitsDef.hasCollision();
				}

				collisionChunk.enabledColliders.set(x, y, z, voxelHasCollision);
			}
		}
	}
}

void CollisionChunkManager::populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	populateChunkShapeDefs(collisionChunk, voxelChunk); // @todo: give positions span that is all WxHxD
	populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give positions span that is all WxHxD

	DebugAssert(collisionChunk.wallCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(collisionChunk.doorCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(collisionChunk.sensorCompoundBodyID == Physics::INVALID_BODY_ID);
	collisionChunk.wallCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Walls, physicsSystem);
	collisionChunk.doorCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Doors, physicsSystem);
	collisionChunk.sensorCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Sensors, physicsSystem);
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	// @todo: this dirty shapes list might be full of 10k brand new voxels this frame, so we're accidentally destroying + recreating them all (found during the AddBodiesPrepare/Finalize() work)

	const BufferView<const VoxelInt3> dirtyShapeDefPositions = voxelChunk.getDirtyShapeDefPositions();
	const BufferView<const VoxelInt3> dirtyDoorAnimInstPositions = voxelChunk.getDirtyDoorAnimInstPositions();

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

		populateChunkShapeDefs(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster
		populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster

		collisionChunk.wallCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Walls, physicsSystem);
		collisionChunk.sensorCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Sensors, physicsSystem);
	}

	if (dirtyDoorAnimInstPositions.getCount() > 0)
	{
		if (collisionChunk.doorCompoundBodyID != Physics::INVALID_BODY_ID)
		{
			bodyInterface.RemoveBody(collisionChunk.doorCompoundBodyID);
			bodyInterface.DestroyBody(collisionChunk.doorCompoundBodyID);
			collisionChunk.doorCompoundBodyID = Physics::INVALID_BODY_ID;
		}

		populateChunkEnabledColliders(collisionChunk, voxelChunk); // @todo: give dirty positions span so it's much faster

		collisionChunk.doorCompoundBodyID = createChunkCompoundShape(collisionChunk, voxelChunk, ceilingScale, CompoundShapeCategory::Doors, physicsSystem);
	}
}

void CollisionChunkManager::update(double dt, BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions, double ceilingScale,
	const VoxelChunkManager &voxelChunkManager, JPH::PhysicsSystem &physicsSystem)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		CollisionChunk &collisionChunk = this->getChunkAtIndex(chunkIndex);
		collisionChunk.freePhysicsCompoundBodies(bodyInterface);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const int spawnIndex = this->spawnChunk();
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->populateChunk(spawnIndex, ceilingScale, chunkPos, voxelChunk, physicsSystem);
	}

	// Update dirty voxels.
	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->updateDirtyVoxels(chunkPos, ceilingScale, voxelChunk, physicsSystem);
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
