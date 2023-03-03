#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <vector>

#include "CollisionMeshDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class CollisionChunk final : public Chunk
{
public:
	using CollisionMeshDefID = int;

	std::vector<CollisionMeshDefinition> meshDefs;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, CollisionChunk::CollisionMeshDefID> meshMappings;
	Buffer3D<CollisionMeshDefID> meshDefIDs;
	Buffer3D<bool> enabledColliders;

	static constexpr CollisionMeshDefID AIR_COLLISION_MESH_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);
	void clear();

	int getCollisionMeshDefCount() const;
	const CollisionMeshDefinition &getCollisionMeshDef(CollisionMeshDefID id) const;
	CollisionMeshDefID addCollisionMeshDef(CollisionMeshDefinition &&meshDef);

	CollisionChunk::CollisionMeshDefID getOrAddMeshDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelMeshDefID voxelMeshDefID);
};

#endif
