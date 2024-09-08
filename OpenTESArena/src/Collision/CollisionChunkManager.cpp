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
	bool TryCreatePhysicsCollider(SNInt x, int y, WEInt z, const ChunkInt2 &chunkPos, const VoxelShapeDefinition &voxelShapeDef,
		const CollisionShapeDefinition &collisionShapeDef, double ceilingScale, JPH::PhysicsSystem &physicsSystem,
		JPH::BodyID *outBodyID)
	{
		JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

		const double voxelYBottom = static_cast<double>(y) * ceilingScale;

		DebugAssert(collisionShapeDef.type == CollisionShapeType::Box);
		const CollisionBoxShapeDefinition &boxShapeDef = collisionShapeDef.box;
		const VoxelShapeScaleType scaleType = voxelShapeDef.scaleType;
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

		JPH::ShapeRefC boxShape = boxShapeResult.Get();
		const WorldInt3 boxWorldVoxelPos = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt3(x, y, z));
		const JPH::RVec3 boxJoltPos(
			static_cast<float>(boxWorldVoxelPos.x + 0.50),
			static_cast<float>(scaledYBottom + scaledHalfHeight),
			static_cast<float>(boxWorldVoxelPos.z + 0.50));
		const JPH::Quat boxJoltQuat = JPH::Quat::sRotation(JPH::Vec3Arg::sAxisY(), boxShapeDef.yRotation);
		const JPH::BodyCreationSettings boxSettings(boxShape, boxJoltPos, boxJoltQuat, JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
		const JPH::Body *box = bodyInterface.CreateBody(boxSettings);
		if (box == nullptr)
		{
			const uint32_t totalBodyCount = physicsSystem.GetNumBodies();
			DebugLogError("Couldn't create Jolt body at (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ") in chunk (" + chunkPos.toString() + ") (total: " + std::to_string(totalBodyCount) + ").");
			return false;
		}

		const JPH::BodyID &boxBodyID = box->GetID();
		bodyInterface.AddBody(boxBodyID, JPH::EActivation::Activate);
		*outBodyID = boxBodyID;
		return true;
	}
}

void CollisionChunkManager::populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	// @todo: Jolt recommends AddBodiesPrepare and AddBodiesFinalize for bulk situations like this
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelChunk::VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(x, y, z);
				const CollisionChunk::CollisionShapeDefID collisionShapeDefID = collisionChunk.getOrAddShapeDefIdMapping(voxelChunk, voxelShapeDefID);
				collisionChunk.shapeDefIDs.set(x, y, z, collisionShapeDefID);

				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(x, y, z);
				const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
				const bool voxelHasCollision = voxelTraitsDef.hasCollision(); // @todo: lore/sound triggers aren't included in this
				collisionChunk.enabledColliders.set(x, y, z, voxelHasCollision);
				
				if (voxelHasCollision)
				{
					const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
					const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

					// Generate box collider and add to the simulation.
					JPH::BodyID bodyID;
					if (TryCreatePhysicsCollider(x, y, z, chunkPos, voxelShapeDef, collisionShapeDef, ceilingScale, physicsSystem, &bodyID))
					{
						collisionChunk.physicsBodyIDs.set(x, y, z, bodyID);
					}
				}
			}
		}
	}
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (const VoxelInt3 &voxelPos : voxelChunk.getDirtyShapeDefPositions())
	{
		const VoxelChunk::VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(voxelPos.x, voxelPos.y, voxelPos.z);
		const CollisionChunk::CollisionShapeDefID collisionShapeDefID = collisionChunk.getOrAddShapeDefIdMapping(voxelChunk, voxelShapeDefID);
		collisionChunk.shapeDefIDs.set(voxelPos.x, voxelPos.y, voxelPos.z, collisionShapeDefID);

		const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(voxelPos.x, voxelPos.y, voxelPos.z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
		const bool voxelHasCollision = voxelTraitsDef.hasCollision();
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, voxelHasCollision);

		JPH::BodyID existingBodyID = collisionChunk.physicsBodyIDs.get(voxelPos.x, voxelPos.y, voxelPos.z);
		if (!existingBodyID.IsInvalid())
		{
			collisionChunk.freePhysicsBodyID(voxelPos.x, voxelPos.y, voxelPos.z, bodyInterface);
		}

		if (voxelHasCollision)
		{
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
			const CollisionShapeDefinition &collisionShapeDef = collisionChunk.getCollisionShapeDef(collisionShapeDefID);

			// Generate box collider and add to the simulation.
			JPH::BodyID bodyID;
			if (TryCreatePhysicsCollider(voxelPos.x, voxelPos.y, voxelPos.z, chunkPos, voxelShapeDef, collisionShapeDef, ceilingScale, physicsSystem, &bodyID))
			{
				collisionChunk.physicsBodyIDs.set(voxelPos.x, voxelPos.y, voxelPos.z, bodyID);
			}
		}
	}

	for (const VoxelInt3 &voxelPos : voxelChunk.getDirtyDoorAnimInstPositions())
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
			bodyInterface.ActivateBody(bodyID);
		}
		else
		{
			bodyInterface.DeactivateBody(bodyID);
		}
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
