#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyLock.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/MutableCompoundShape.h"

#include "CollisionChunkManager.h"
#include "CollisionShapeDefinition.h"
#include "Physics.h"
#include "PhysicsLayer.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/MeshUtils.h"

namespace
{
	JPH::MutableCompoundShape *GetShapeAsCompoundShape(JPH::Shape *shape)
	{
		return static_cast<JPH::MutableCompoundShape*>(shape);
	}

	JPH::MutableCompoundShape *GetCompoundShapeFromBody(JPH::BodyID bodyID, JPH::PhysicsSystem &physicsSystem)
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
		JPH::MutableCompoundShape *mutShape = GetShapeAsCompoundShape(const_cast<JPH::Shape*>(physicsCompoundBody.GetShape()));
		return mutShape;
	}

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
}

void CollisionChunkManager::populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	JPH::MutableCompoundShapeSettings nonMovingCompoundSettings;
	JPH::MutableCompoundShapeSettings sensorCompoundSettings;
	nonMovingCompoundSettings.SetEmbedded();
	sensorCompoundSettings.SetEmbedded();

	std::vector<JPH::Ref<JPH::BoxShapeSettings>> boxShapeSettingsList; // Freed at end of scope

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(x, y, z);
				const CollisionShapeDefID collisionShapeDefID = collisionChunk.getOrAddShapeDefIdMapping(voxelChunk, voxelShapeDefID);
				collisionChunk.shapeDefIDs.set(x, y, z, collisionShapeDefID);

				const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(x, y, z);
				const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
				const bool voxelHasCollision = voxelTraitsDef.hasCollision();
				collisionChunk.enabledColliders.set(x, y, z, voxelHasCollision);

				bool isTriggerVoxel = false;
				VoxelTriggerDefID triggerDefID;
				if (voxelChunk.tryGetTriggerDefID(x, y, z, &triggerDefID))
				{
					const VoxelTriggerDefinition &voxelTriggerDef = voxelChunk.getTriggerDef(triggerDefID);
					isTriggerVoxel = voxelTriggerDef.hasValidDef();
				}

				bool isInteriorLevelChangeVoxel = false;
				VoxelTransitionDefID transitionDefID;
				if (voxelChunk.tryGetTransitionDefID(x, y, z, &transitionDefID))
				{
					const TransitionDefinition &transitionDef = voxelChunk.getTransitionDef(transitionDefID);
					isInteriorLevelChangeVoxel = transitionDef.type == TransitionType::InteriorLevelChange;
				}

				const bool isSensorCollider = isTriggerVoxel || isInteriorLevelChangeVoxel;
				const bool shouldCreateCollider = voxelHasCollision || isSensorCollider;

				if (shouldCreateCollider)
				{
					const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
					const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

					boxShapeSettingsList.emplace_back(new JPH::BoxShapeSettings());
					JPH::BoxShapeSettings *boxShapeSettings = boxShapeSettingsList.back().GetPtr();

					JPH::Vec3 boxPosition;
					JPH::Quat boxRotation;
					MakePhysicsColliderInitValues(x, y, z, chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, boxShapeSettings, &boxPosition, &boxRotation);

					if (isSensorCollider)
					{
						sensorCompoundSettings.AddShape(boxPosition, boxRotation, boxShapeSettings);
					}
					else
					{
						nonMovingCompoundSettings.AddShape(boxPosition, boxRotation, boxShapeSettings);
					}

					//collisionChunk.physicsSubShapeIDs.set(x, y, z, bodyID); // @todo: not sure how to create/get subshapeid at this stage
				}
			}
		}
	}

	const JPH::Vec3 compoundBodyPosition = JPH::Vec3::sZero();
	const JPH::Quat compoundBodyRotation = JPH::Quat::sIdentity();
	JPH::BodyCreationSettings nonMovingCompoundBodyCreationSettings(&nonMovingCompoundSettings, compoundBodyPosition, compoundBodyRotation, JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
	JPH::BodyCreationSettings sensorCompoundBodyCreationSettings(&sensorCompoundSettings, compoundBodyPosition, compoundBodyRotation, JPH::EMotionType::Static, PhysicsLayers::SENSOR);
	
	// Keep player from erratically hopping/skipping when running due to no contact welding in Jolt.
	nonMovingCompoundBodyCreationSettings.mEnhancedInternalEdgeRemoval = true;

	sensorCompoundBodyCreationSettings.mIsSensor = true;

	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
	collisionChunk.nonMovingCompoundBodyID = bodyInterface.CreateAndAddBody(nonMovingCompoundBodyCreationSettings, JPH::EActivation::Activate);
	collisionChunk.sensorCompoundBodyID = bodyInterface.CreateAndAddBody(sensorCompoundBodyCreationSettings, JPH::EActivation::Activate);

	// @todo: set all collisionChunk.physicsSubShapeIDs?
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	const BufferView<const VoxelInt3> dirtyShapeDefPositions = voxelChunk.getDirtyShapeDefPositions();
	const BufferView<const VoxelInt3> dirtyDoorAnimInstPositions = voxelChunk.getDirtyDoorAnimInstPositions();

	JPH::MutableCompoundShape *nonMovingCompoundShape = GetCompoundShapeFromBody(collisionChunk.nonMovingCompoundBodyID, physicsSystem);

	/*std::vector<JPH::BodyID> bodyIDsToAdd;
	std::vector<JPH::BodyID> bodyIDsToRemove;
	std::vector<JPH::BodyID> bodyIDsToDestroy;
	bodyIDsToAdd.reserve(dirtyShapeDefPositions.getCount());
	bodyIDsToRemove.reserve(dirtyDoorAnimInstPositions.getCount());
	bodyIDsToDestroy.reserve(dirtyShapeDefPositions.getCount());*/

	// @todo: this dirty shapes list might be full of brand new voxels this frame, so we're accidentally destroying + recreating them all (found during the AddBodiesPrepare/Finalize() work)
	for (const VoxelInt3 &voxelPos : dirtyShapeDefPositions)
	{
		const SNInt x = voxelPos.x;
		const int y = voxelPos.y;
		const WEInt z = voxelPos.z;
		const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(x, y, z);
		const CollisionShapeDefID collisionShapeDefID = collisionChunk.getOrAddShapeDefIdMapping(voxelChunk, voxelShapeDefID);
		collisionChunk.shapeDefIDs.set(x, y, z, collisionShapeDefID);

		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(x, y, z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
		const bool voxelHasCollision = voxelTraitsDef.hasCollision();
		collisionChunk.enabledColliders.set(x, y, z, voxelHasCollision);

		// @todo
		/*JPH::BodyID existingBodyID = collisionChunk.physicsBodyIDs.get(x, y, z);
		if (!existingBodyID.IsInvalid())
		{
			if (bodyInterface.IsAdded(existingBodyID))
			{
				bodyIDsToRemove.emplace_back(existingBodyID);
			}

			bodyIDsToDestroy.emplace_back(existingBodyID);
			collisionChunk.physicsBodyIDs.set(x, y, z, Physics::INVALID_BODY_ID);
		}*/

		bool isTriggerVoxel = false;
		VoxelTriggerDefID triggerDefID;
		if (voxelChunk.tryGetTriggerDefID(x, y, z, &triggerDefID))
		{
			const VoxelTriggerDefinition &voxelTriggerDef = voxelChunk.getTriggerDef(triggerDefID);
			isTriggerVoxel = voxelTriggerDef.hasValidDef();
		}

		bool isInteriorLevelChangeVoxel = false;
		VoxelTransitionDefID transitionDefID;
		if (voxelChunk.tryGetTransitionDefID(x, y, z, &transitionDefID))
		{
			const TransitionDefinition &transitionDef = voxelChunk.getTransitionDef(transitionDefID);
			isInteriorLevelChangeVoxel = transitionDef.type == TransitionType::InteriorLevelChange;
		}

		const bool isSensorCollider = isTriggerVoxel || isInteriorLevelChangeVoxel;
		const bool shouldCreateCollider = voxelHasCollision || isSensorCollider;

		if (shouldCreateCollider)
		{
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
			const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

			// @todo

			/*// Generate collider but don't add to simulation yet.
			JPH::BodyID bodyID;
			if (TryCreatePhysicsColliderInitInfo(x, y, z, chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, &bodyID))
			{
				collisionChunk.physicsBodyIDs.set(x, y, z, bodyID);
			}*/

			//bodyIDsToAdd.emplace_back(bodyID);
		}
	}

	for (const VoxelInt3 &voxelPos : dirtyDoorAnimInstPositions)
	{
		int doorAnimInstIndex;
		const bool success = voxelChunk.tryGetDoorAnimInstIndex(voxelPos.x, voxelPos.y, voxelPos.z, &doorAnimInstIndex);
		DebugAssertMsg(success, "Expected door anim inst to be available for (" + voxelPos.toString() + ").");
		
		const BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
		const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
		const bool shouldEnableDoorCollider = doorAnimInst.stateType == VoxelDoorAnimationInstance::StateType::Closed;
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, shouldEnableDoorCollider);

		/*const JPH::BodyID &bodyID = collisionChunk.physicsBodyIDs.get(voxelPos.x, voxelPos.y, voxelPos.z);
		DebugAssertMsg(!bodyID.IsInvalid(), "Expected valid Jolt body for door voxel at (" + voxelPos.toString() + ") in chunk (" + chunkPos.toString() + ").");*/

		if (shouldEnableDoorCollider)
		{
			/*if (!bodyInterface.IsAdded(bodyID))
			{
				bodyIDsToAdd.emplace_back(bodyID);
			}*/
		}
		else
		{
			/*if (bodyInterface.IsAdded(bodyID))
			{
				bodyIDsToRemove.emplace_back(bodyID);
			}*/
		}
	}

	// Do bulk adds/removes for efficiency.
	/*if (!bodyIDsToAdd.empty())
	{
		const int bodyIDAddCount = static_cast<int>(bodyIDsToAdd.size());
		JPH::BodyInterface::AddState bodyAddState = bodyInterface.AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDAddCount);
		bodyInterface.AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDAddCount, bodyAddState, JPH::EActivation::Activate);
	}
	
	if (!bodyIDsToRemove.empty())
	{
		bodyInterface.RemoveBodies(bodyIDsToRemove.data(), static_cast<int>(bodyIDsToRemove.size()));
	}

	if (!bodyIDsToDestroy.empty())
	{
		bodyInterface.DestroyBodies(bodyIDsToDestroy.data(), static_cast<int>(bodyIDsToDestroy.size()));
	}*/
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
