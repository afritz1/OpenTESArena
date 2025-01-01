#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSettings.h"

#include "CollisionChunk.h"
#include "Physics.h"
#include "../Voxels/ArenaChasmUtils.h"

#include "components/debug/Debug.h"

void CollisionChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first definition (air) be usable immediately. All default IDs can safely point to it.
	this->shapeDefs.emplace_back(CollisionShapeDefinition());
	this->shapeMappings.emplace(VoxelChunk::AIR_TRAITS_DEF_ID, CollisionChunk::AIR_COLLISION_SHAPE_DEF_ID);

	this->shapeDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->shapeDefIDs.fill(CollisionChunk::AIR_COLLISION_SHAPE_DEF_ID);

	this->enabledColliders.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->enabledColliders.fill(false);

	this->physicsBodyIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->physicsBodyIDs.fill(Physics::INVALID_BODY_ID);
}

void CollisionChunk::freePhysicsBodyIDs(JPH::BodyInterface &bodyInterface)
{
	const SNInt chunkWidth = this->physicsBodyIDs.getWidth();
	const int chunkHeight = this->physicsBodyIDs.getHeight();
	const WEInt chunkDepth = this->physicsBodyIDs.getDepth();
	const int estimatedBodyIDCount = chunkWidth * chunkHeight * chunkDepth;

	std::vector<JPH::BodyID> bodyIDsToRemove;
	std::vector<JPH::BodyID> bodyIDsToDestroy;
	bodyIDsToRemove.reserve(estimatedBodyIDCount);
	bodyIDsToDestroy.reserve(estimatedBodyIDCount);

	for (WEInt z = 0; z < chunkDepth; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < chunkWidth; x++)
			{
				JPH::BodyID &bodyID = this->physicsBodyIDs.get(x, y, z);
				if (!bodyID.IsInvalid())
				{
					if (bodyInterface.IsAdded(bodyID))
					{
						bodyIDsToRemove.emplace_back(bodyID);
					}

					bodyIDsToDestroy.emplace_back(bodyID);
					bodyID = Physics::INVALID_BODY_ID;
				}
			}
		}
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

void CollisionChunk::clear()
{
	Chunk::clear();
	this->shapeDefs.clear();
	this->shapeMappings.clear();
	this->shapeDefIDs.clear();
	this->enabledColliders.clear();
	this->physicsBodyIDs.clear();
}

int CollisionChunk::getCollisionShapeDefCount() const
{
	return static_cast<int>(this->shapeDefs.size());
}

const CollisionShapeDefinition &CollisionChunk::getCollisionShapeDef(CollisionShapeDefID id) const
{
	DebugAssertIndex(this->shapeDefs, id);
	return this->shapeDefs[id];
}

CollisionChunk::CollisionShapeDefID CollisionChunk::addCollisionShapeDef(CollisionShapeDefinition &&shapeDef)
{
	const CollisionShapeDefID id = static_cast<CollisionShapeDefID>(this->shapeDefs.size());
	this->shapeDefs.emplace_back(std::move(shapeDef));
	return id;
}

CollisionChunk::CollisionShapeDefID CollisionChunk::getOrAddShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelShapeDefID voxelShapeDefID)
{
	CollisionChunk::CollisionShapeDefID collisionShapeDefID = -1;

	const auto iter = this->shapeMappings.find(voxelShapeDefID);
	if (iter != this->shapeMappings.end())
	{
		collisionShapeDefID = iter->second;
	}
	else
	{
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);

		CollisionShapeDefinition collisionShapeDef;
		if (voxelShapeDef.type == VoxelShapeType::Box)
		{
			const VoxelBoxShapeDefinition &voxelBoxShapeDef = voxelShapeDef.box;
			collisionShapeDef.initBox(voxelBoxShapeDef.width, voxelBoxShapeDef.height, voxelBoxShapeDef.depth, voxelBoxShapeDef.yOffset, voxelBoxShapeDef.yRotation);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelShapeDef.type)));
		}
		
		collisionShapeDefID = this->addCollisionShapeDef(std::move(collisionShapeDef));
		this->shapeMappings.emplace(voxelShapeDefID, collisionShapeDefID);
	}

	return collisionShapeDefID;
}
