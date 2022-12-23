#include "CollisionChunk.h"

void CollisionChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	
	this->meshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->meshDefIDs.fill(CollisionChunk::AIR_COLLISION_MESH_DEF_ID);

	this->enabledColliders.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->enabledColliders.fill(false);
}
