#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"

#include "CollisionChunkManager.h"
#include "CollisionShapeDefinition.h"
#include "Physics.h"
#include "PhysicsLayer.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/MeshUtils.h"

namespace
{
	// Creates collider but does not add it to simulation.
	bool TryCreatePhysicsCollider(SNInt x, int y, WEInt z, const ChunkInt2 &chunkPos, const CollisionShapeDefinition &collisionShapeDef,
		VoxelShapeScaleType scaleType, double ceilingScale, bool isSensor, JPH::PhysicsSystem &physicsSystem, JPH::BodyID *outBodyID)
	{
		const double voxelYBottom = static_cast<double>(y) * ceilingScale;

		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &boxShapeDef = collisionShapeDef.box;
		const double scaledYBottom = voxelYBottom + MeshUtils::getScaledVertexY(boxShapeDef.yOffset, scaleType, ceilingScale);
		const double scaledYTop = voxelYBottom + MeshUtils::getScaledVertexY(boxShapeDef.yOffset + boxShapeDef.height, scaleType, ceilingScale);
		const double scaledHeight = scaledYTop - scaledYBottom;
		const double scaledHalfHeight = scaledHeight * 0.50;

		const JPH::Vec3 boxShapeHalfThicknesses(
			static_cast<float>(boxShapeDef.width * 0.50),
			static_cast<float>(scaledHalfHeight),
			static_cast<float>(boxShapeDef.depth * 0.50));
		constexpr float boxShapeConvexRadius = static_cast<float>(Physics::BoxConvexRadius);

		JPH::BoxShapeSettings boxShapeSettings(boxShapeHalfThicknesses, boxShapeConvexRadius);
		boxShapeSettings.SetEmbedded(); // Marked embedded to prevent it from being freed when its ref count reaches 0.
		// @todo: make sure this ^ isn't leaking when we remove/destroy the body

		JPH::ShapeSettings::ShapeResult boxShapeResult = boxShapeSettings.Create();
		if (boxShapeResult.HasError())
		{
			DebugLogError("Couldn't create Jolt box collider settings: " + std::string(boxShapeResult.GetError().c_str()));
			return false;
		}

		JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
		JPH::ShapeRefC boxShape = boxShapeResult.Get();
		const WorldInt3 boxWorldVoxelPos = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt3(x, y, z));
		const JPH::RVec3 boxJoltPos(
			static_cast<float>(boxWorldVoxelPos.x + 0.50),
			static_cast<float>(scaledYBottom + scaledHalfHeight),
			static_cast<float>(boxWorldVoxelPos.z + 0.50));
		const RadiansF boxYRotation = static_cast<RadiansF>(boxShapeDef.yRotation);
		const JPH::Quat boxJoltQuat = JPH::Quat::sRotation(JPH::Vec3Arg::sAxisY(), boxYRotation);
		const JPH::BodyCreationSettings boxSettings(boxShape, boxJoltPos, boxJoltQuat, JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
		JPH::Body *boxBody = bodyInterface.CreateBody(boxSettings);
		if (boxBody == nullptr)
		{
			const uint32_t totalBodyCount = physicsSystem.GetNumBodies();
			DebugLogError("Couldn't create Jolt body at (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ") in chunk (" + chunkPos.toString() + ") (total: " + std::to_string(totalBodyCount) + ").");
			return false;
		}

		boxBody->SetIsSensor(isSensor);
		*outBodyID = boxBody->GetID();
		return true;
	}
}

void CollisionChunkManager::populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	std::vector<JPH::BodyID> createdBodyIDs;
	createdBodyIDs.reserve(Chunk::WIDTH * chunkHeight * Chunk::DEPTH);

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

				VoxelTriggerDefID triggerDefID;
				const bool isTriggerVoxel = voxelChunk.tryGetTriggerDefID(x, y, z, &triggerDefID);

				VoxelTransitionDefID transitionDefID;
				const bool isTransitionVoxel = voxelChunk.tryGetTransitionDefID(x, y, z, &transitionDefID);
				const bool isSensorCollider = isTriggerVoxel || isTransitionVoxel;
				const bool shouldCreateCollider = voxelHasCollision || isSensorCollider;

				if (shouldCreateCollider)
				{
					const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
					const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

					// Generate collider but don't add to simulation.
					JPH::BodyID bodyID;
					if (TryCreatePhysicsCollider(x, y, z, chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, &bodyID))
					{
						collisionChunk.physicsBodyIDs.set(x, y, z, bodyID);
						createdBodyIDs.emplace_back(bodyID);
					}
				}
			}
		}
	}

	const int createdBodyIDCount = static_cast<int>(createdBodyIDs.size());

	// Add bodies to the simulation in bulk for efficiency.
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
	JPH::BodyInterface::AddState bodyAddState = bodyInterface.AddBodiesPrepare(createdBodyIDs.data(), createdBodyIDCount);
	bodyInterface.AddBodiesFinalize(createdBodyIDs.data(), createdBodyIDCount, bodyAddState, JPH::EActivation::Activate);
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	const BufferView<const VoxelInt3> dirtyShapeDefPositions = voxelChunk.getDirtyShapeDefPositions();
	const BufferView<const VoxelInt3> dirtyDoorAnimInstPositions = voxelChunk.getDirtyDoorAnimInstPositions();

	std::vector<JPH::BodyID> bodyIDsToAdd;
	std::vector<JPH::BodyID> bodyIDsToRemove;
	std::vector<JPH::BodyID> bodyIDsToDestroy;
	bodyIDsToAdd.reserve(dirtyShapeDefPositions.getCount());
	bodyIDsToRemove.reserve(dirtyDoorAnimInstPositions.getCount());
	bodyIDsToDestroy.reserve(dirtyShapeDefPositions.getCount());

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

		JPH::BodyID existingBodyID = collisionChunk.physicsBodyIDs.get(x, y, z);
		if (!existingBodyID.IsInvalid())
		{
			if (bodyInterface.IsAdded(existingBodyID))
			{
				bodyIDsToRemove.emplace_back(existingBodyID);
			}

			bodyIDsToDestroy.emplace_back(existingBodyID);
			collisionChunk.physicsBodyIDs.set(x, y, z, Physics::INVALID_BODY_ID);
		}

		VoxelTriggerDefID triggerDefID;
		const bool isTriggerVoxel = voxelChunk.tryGetTriggerDefID(x, y, z, &triggerDefID);

		VoxelTransitionDefID transitionDefID;
		const bool isTransitionVoxel = voxelChunk.tryGetTransitionDefID(x, y, z, &transitionDefID);
		const bool isSensorCollider = isTriggerVoxel || isTransitionVoxel;
		const bool shouldCreateCollider = voxelHasCollision || isSensorCollider;

		if (shouldCreateCollider)
		{
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
			const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

			// Generate collider but don't add to simulation yet.
			JPH::BodyID bodyID;
			if (TryCreatePhysicsCollider(x, y, z, chunkPos, collisionShapeDef, voxelShapeDef.scaleType, ceilingScale, isSensorCollider, physicsSystem, &bodyID))
			{
				collisionChunk.physicsBodyIDs.set(x, y, z, bodyID);
			}

			bodyIDsToAdd.emplace_back(bodyID);
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

		const JPH::BodyID &bodyID = collisionChunk.physicsBodyIDs.get(voxelPos.x, voxelPos.y, voxelPos.z);
		DebugAssertMsg(!bodyID.IsInvalid(), "Expected valid Jolt body for door voxel at (" + voxelPos.toString() + ") in chunk (" + chunkPos.toString() + ").");

		if (shouldEnableDoorCollider)
		{
			if (!bodyInterface.IsAdded(bodyID))
			{
				bodyIDsToAdd.emplace_back(bodyID);
			}
		}
		else
		{
			if (bodyInterface.IsAdded(bodyID))
			{
				bodyIDsToRemove.emplace_back(bodyID);
			}
		}
	}

	// Do bulk adds/removes for efficiency.
	if (!bodyIDsToAdd.empty())
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
		collisionChunk.freePhysicsBodyIDs(bodyInterface);
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
		chunkPtr->freePhysicsBodyIDs(bodyInterface);
		this->recycleChunk(i);
	}
}
