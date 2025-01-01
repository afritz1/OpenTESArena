#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyInterface.h"

#include "CollisionShapeDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class CollisionChunk final : public Chunk
{
public:
	using CollisionShapeDefID = int;

	std::vector<CollisionShapeDefinition> shapeDefs;
	std::unordered_map<VoxelShapeDefID, CollisionChunk::CollisionShapeDefID> shapeMappings;
	Buffer3D<CollisionShapeDefID> shapeDefIDs;
	Buffer3D<bool> enabledColliders; // @todo: decide if this is obsolete and whether the Body can store its in/out of world state
	Buffer3D<JPH::BodyID> physicsBodyIDs;

	static constexpr CollisionShapeDefID AIR_COLLISION_SHAPE_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);
	void freePhysicsBodyIDs(JPH::BodyInterface &bodyInterface);
	void clear();

	int getCollisionShapeDefCount() const;
	const CollisionShapeDefinition &getCollisionShapeDef(CollisionShapeDefID id) const;
	CollisionShapeDefID addCollisionShapeDef(CollisionShapeDefinition &&shapeDef);
	CollisionChunk::CollisionShapeDefID getOrAddShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelShapeDefID voxelShapeDefID);
};

#endif
