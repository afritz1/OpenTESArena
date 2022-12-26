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
	std::vector<CollisionMeshDefinition> meshDefs;
	Buffer3D<CollisionMeshDefID> meshDefIDs;
	Buffer3D<bool> enabledColliders;
public:
	static constexpr CollisionMeshDefID AIR_COLLISION_MESH_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);

	int getCollisionMeshDefCount() const;
	const CollisionMeshDefinition &getCollisionMeshDef(CollisionMeshDefID id) const;
	CollisionMeshDefID addCollisionMeshDef(CollisionMeshDefinition &&meshDef);

	CollisionMeshDefID getCollisionMeshDefID(SNInt x, int y, WEInt z) const;
	void setCollisionMeshDefID(SNInt x, int y, WEInt z, CollisionMeshDefID id);

	bool isColliderEnabled(SNInt x, int y, WEInt z) const;
	void setColliderEnabled(SNInt x, int y, WEInt z, bool enabled);
};

#endif
