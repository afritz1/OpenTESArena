#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <unordered_map>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyInterface.h"

#include "CollisionShapeDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

using CollisionShapeDefID = int;

struct CollisionChunk final : public Chunk
{
	std::vector<CollisionShapeDefinition> shapeDefs;
	std::unordered_map<VoxelShapeDefID, CollisionShapeDefID> shapeMappings;
	Buffer3D<CollisionShapeDefID> shapeDefIDs;
	Buffer3D<bool> enabledColliders; // @todo: decide if this is obsolete and whether the Body can store its in/out of world state
	
	JPH::BodyID wallCompoundBodyID; // Holds most of the game world, uses enhanced internal edge removal setting.
	JPH::BodyID doorCompoundBodyID; // Holds door colliders
	JPH::BodyID sensorCompoundBodyID; // Holds SENSOR colliders

	static constexpr CollisionShapeDefID AIR_COLLISION_SHAPE_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);
	void freePhysicsCompoundBodies(JPH::BodyInterface &bodyInterface);
	void clear();

	int getCollisionShapeDefCount() const;
	const CollisionShapeDefinition &getCollisionShapeDef(CollisionShapeDefID id) const;
	CollisionShapeDefID findShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelShapeDefID voxelShapeDefID) const;
	CollisionShapeDefID addShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelShapeDefID voxelShapeDefID);
};

#endif
