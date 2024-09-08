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

void CollisionChunk::freePhysicsBodyID(SNInt x, int y, WEInt z, JPH::BodyInterface &bodyInterface)
{
	JPH::BodyID &bodyID = this->physicsBodyIDs.get(x, y, z);
	if (!bodyID.IsInvalid())
	{
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);
		bodyID = Physics::INVALID_BODY_ID;
	}
}

void CollisionChunk::freePhysicsBodyIDs(JPH::BodyInterface &bodyInterface)
{
	// @todo optimize
	for (WEInt z = 0; z < this->physicsBodyIDs.getDepth(); z++)
	{
		for (int y = 0; y < this->physicsBodyIDs.getHeight(); y++)
		{
			for (SNInt x = 0; x < this->physicsBodyIDs.getWidth(); x++)
			{
				this->freePhysicsBodyID(x, y, z, bodyInterface);
			}
		}
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
		if (voxelShapeDef.type == VoxelShapeType::None)
		{
			collisionShapeDef.initNone();
		}
		else
		{
			const VoxelBoxShapeDefinition &voxelBoxShapeDef = voxelShapeDef.box;
			collisionShapeDef.initBox(voxelBoxShapeDef.width, voxelBoxShapeDef.height, voxelBoxShapeDef.depth, voxelBoxShapeDef.yOffset, voxelBoxShapeDef.yRotation);
		}
		
		collisionShapeDefID = this->addCollisionShapeDef(std::move(collisionShapeDef));
		this->shapeMappings.emplace(voxelShapeDefID, collisionShapeDefID);
	}

	return collisionShapeDefID;
}
