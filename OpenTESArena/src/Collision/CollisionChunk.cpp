#include "CollisionChunk.h"

#include "components/debug/Debug.h"

void CollisionChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first definition (air) be usable immediately. All default IDs can safely point to it.
	this->meshDefs.emplace_back(CollisionMeshDefinition());

	this->meshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->meshDefIDs.fill(CollisionChunk::AIR_COLLISION_MESH_DEF_ID);

	this->enabledColliders.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->enabledColliders.fill(false);
}

void CollisionChunk::clear()
{
	Chunk::clear();
	this->meshDefs.clear();
	this->meshDefIDs.clear();
	this->enabledColliders.clear();
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

CollisionChunk::CollisionMeshDefID CollisionChunk::getCollisionMeshDefID(SNInt x, int y, WEInt z) const
{
	return this->meshDefIDs.get(x, y, z);
}

void CollisionChunk::setCollisionMeshDefID(SNInt x, int y, WEInt z, CollisionMeshDefID id)
{
	this->meshDefIDs.set(x, y, z, id);
}

bool CollisionChunk::isColliderEnabled(SNInt x, int y, WEInt z) const
{
	return this->enabledColliders.get(x, y, z);
}

void CollisionChunk::setColliderEnabled(SNInt x, int y, WEInt z, bool enabled)
{
	this->enabledColliders.set(x, y, z, enabled);
}
