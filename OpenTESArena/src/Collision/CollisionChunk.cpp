#include "Jolt/Jolt.h"

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

	this->wallCompoundBodyID = Physics::INVALID_BODY_ID;
	this->doorCompoundBodyID = Physics::INVALID_BODY_ID;
	this->sensorCompoundBodyID = Physics::INVALID_BODY_ID;
}

void CollisionChunk::freePhysicsCompoundBodies(JPH::BodyInterface &bodyInterface)
{
	if (!this->wallCompoundBodyID.IsInvalid())
	{
		bodyInterface.RemoveBody(this->wallCompoundBodyID);
		bodyInterface.DestroyBody(this->wallCompoundBodyID);
		this->wallCompoundBodyID = Physics::INVALID_BODY_ID;
	}

	if (!this->doorCompoundBodyID.IsInvalid())
	{
		bodyInterface.RemoveBody(this->doorCompoundBodyID);
		bodyInterface.DestroyBody(this->doorCompoundBodyID);
		this->doorCompoundBodyID = Physics::INVALID_BODY_ID;
	}

	if (!this->sensorCompoundBodyID.IsInvalid())
	{
		bodyInterface.RemoveBody(this->sensorCompoundBodyID);
		bodyInterface.DestroyBody(this->sensorCompoundBodyID);
		this->sensorCompoundBodyID = Physics::INVALID_BODY_ID;
	}
}

void CollisionChunk::clear()
{
	Chunk::clear();
	this->shapeDefs.clear();
	this->shapeMappings.clear();
	this->shapeDefIDs.clear();
	this->enabledColliders.clear();
	DebugAssert(this->wallCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(this->doorCompoundBodyID == Physics::INVALID_BODY_ID);
	DebugAssert(this->sensorCompoundBodyID == Physics::INVALID_BODY_ID);
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

CollisionShapeDefID CollisionChunk::findShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelShapeDefID voxelShapeDefID) const
{
	const auto iter = this->shapeMappings.find(voxelShapeDefID);
	if (iter == this->shapeMappings.end())
	{
		return -1;
	}

	return iter->second;
}

CollisionShapeDefID CollisionChunk::addShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelShapeDefID voxelShapeDefID)
{
	const auto iter = this->shapeMappings.find(voxelShapeDefID);
	if (iter != this->shapeMappings.end())
	{
		DebugLogError("CollisionShapeDefID mapping already exists for chunk (" + this->getPosition().toString() + ").");
		return iter->second;
	}

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

	CollisionShapeDefID collisionShapeDefID = static_cast<CollisionShapeDefID>(this->shapeDefs.size());
	this->shapeDefs.emplace_back(std::move(collisionShapeDef));
	this->shapeMappings.emplace(voxelShapeDefID, collisionShapeDefID);

	return collisionShapeDefID;
}
