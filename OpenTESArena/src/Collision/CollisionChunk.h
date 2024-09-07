#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyInterface.h"

#include "CollisionMeshDefinition.h"
#include "CollisionShapeDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class CollisionChunk final : public Chunk
{
public:
	using CollisionMeshDefID = int;
	using CollisionShapeDefID = int;

	std::vector<CollisionMeshDefinition> meshDefs;
	std::vector<CollisionShapeDefinition> shapeDefs; // @todo: CollisionShapeDefinition could probably have a mesh inside it as one of the shapes it supports, but not important
	std::unordered_map<VoxelChunk::VoxelMeshDefID, CollisionChunk::CollisionMeshDefID> meshMappings;
	std::unordered_map<VoxelChunk::VoxelTraitsDefID, CollisionChunk::CollisionShapeDefID> shapeMappings;
	Buffer3D<CollisionMeshDefID> meshDefIDs;
	Buffer3D<CollisionShapeDefID> shapeDefIDs;
	Buffer3D<bool> enabledColliders; // @todo: decide if this is obsolete and whether the Body can store its in/out of world state
	Buffer3D<JPH::BodyID> physicsBodyIDs;

	static constexpr CollisionMeshDefID AIR_COLLISION_MESH_DEF_ID = 0;
	static constexpr CollisionShapeDefID AIR_COLLISION_SHAPE_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);
	void freePhysicsBodyID(SNInt x, int y, WEInt z, JPH::BodyInterface &bodyInterface);
	void freePhysicsBodyIDs(JPH::BodyInterface &bodyInterface);
	void clear();

	int getCollisionMeshDefCount() const;
	int getCollisionShapeDefCount() const;
	const CollisionMeshDefinition &getCollisionMeshDef(CollisionMeshDefID id) const;
	const CollisionShapeDefinition &getCollisionShapeDef(CollisionShapeDefID id) const;
	CollisionMeshDefID addCollisionMeshDef(CollisionMeshDefinition &&meshDef);
	CollisionShapeDefID addCollisionShapeDef(CollisionShapeDefinition &&shapeDef);

	CollisionChunk::CollisionMeshDefID getOrAddMeshDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelMeshDefID voxelMeshDefID);
	CollisionChunk::CollisionShapeDefID getOrAddShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelTraitsDefID voxelTraitsDefID);
};

#endif
