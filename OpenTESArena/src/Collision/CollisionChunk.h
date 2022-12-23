#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <vector>

#include "CollisionMeshDefinition.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class CollisionChunk final : public Chunk
{
public:
	using CollisionMeshDefID = int;
private:
	Buffer3D<CollisionMeshDefID> meshDefIDs;
	Buffer3D<bool> enabledColliders;
public:
	static constexpr CollisionMeshDefID AIR_COLLISION_MESH_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);
};

#endif
