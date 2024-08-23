#include "CollisionChunk.h"
#include "Physics.h"

#include "components/debug/Debug.h"

void CollisionChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first definition (air) be usable immediately. All default IDs can safely point to it.
	this->meshDefs.emplace_back(CollisionMeshDefinition());
	this->meshMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, CollisionChunk::AIR_COLLISION_MESH_DEF_ID);

	this->meshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->meshDefIDs.fill(CollisionChunk::AIR_COLLISION_MESH_DEF_ID);

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
	this->meshDefs.clear();
	this->meshMappings.clear();
	this->meshDefIDs.clear();
	this->enabledColliders.clear();
	this->physicsBodyIDs.clear();
}

int CollisionChunk::getCollisionMeshDefCount() const
{
	return static_cast<int>(this->meshDefs.size());
}

const CollisionMeshDefinition &CollisionChunk::getCollisionMeshDef(CollisionMeshDefID id) const
{
	DebugAssertIndex(this->meshDefs, id);
	return this->meshDefs[id];
}

CollisionChunk::CollisionMeshDefID CollisionChunk::addCollisionMeshDef(CollisionMeshDefinition &&meshDef)
{
	const CollisionMeshDefID id = static_cast<CollisionMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(meshDef));
	return id;
}

CollisionChunk::CollisionMeshDefID CollisionChunk::getOrAddMeshDefIdMapping(const VoxelChunk &voxelChunk,
	VoxelChunk::VoxelMeshDefID voxelMeshDefID)
{
	CollisionChunk::CollisionMeshDefID collisionMeshDefID = -1;

	const auto iter = this->meshMappings.find(voxelMeshDefID);
	if (iter != this->meshMappings.end())
	{
		collisionMeshDefID = iter->second;
	}
	else
	{
		const VoxelMeshDefinition &voxelMeshDef = voxelChunk.getMeshDef(voxelMeshDefID);
		const BufferView<const double> verticesView(voxelMeshDef.collisionVertices);
		const BufferView<const double> normalsView(voxelMeshDef.collisionNormals);
		const BufferView<const int> indicesView(voxelMeshDef.collisionIndices);

		CollisionMeshDefinition collisionMeshDef;
		collisionMeshDef.init(verticesView, normalsView, indicesView);
		collisionMeshDefID = this->addCollisionMeshDef(std::move(collisionMeshDef));
		this->meshMappings.emplace(voxelMeshDefID, collisionMeshDefID);
	}

	return collisionMeshDefID;
}
