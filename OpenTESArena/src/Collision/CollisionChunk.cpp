#include "CollisionChunk.h"
#include "Physics.h"
#include "../Voxels/ArenaChasmUtils.h"

#include "components/debug/Debug.h"

void CollisionChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	// Let the first definition (air) be usable immediately. All default IDs can safely point to it.
	this->meshDefs.emplace_back(CollisionMeshDefinition());
	this->shapeDefs.emplace_back(CollisionShapeDefinition());
	this->meshMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, CollisionChunk::AIR_COLLISION_MESH_DEF_ID);
	this->shapeMappings.emplace(VoxelChunk::AIR_TRAITS_DEF_ID, CollisionChunk::AIR_COLLISION_SHAPE_DEF_ID);

	this->meshDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->meshDefIDs.fill(CollisionChunk::AIR_COLLISION_MESH_DEF_ID);

	this->shapeDefIDs.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->shapeDefIDs.fill(CollisionChunk::AIR_COLLISION_SHAPE_DEF_ID);

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
	// @todo optimize
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
	this->shapeDefs.clear();
	this->meshMappings.clear();
	this->shapeMappings.clear();
	this->meshDefIDs.clear();
	this->shapeDefIDs.clear();
	this->enabledColliders.clear();
	this->physicsBodyIDs.clear();
}

int CollisionChunk::getCollisionMeshDefCount() const
{
	return static_cast<int>(this->meshDefs.size());
}

int CollisionChunk::getCollisionShapeDefCount() const
{
	return static_cast<int>(this->shapeDefs.size());
}

const CollisionMeshDefinition &CollisionChunk::getCollisionMeshDef(CollisionMeshDefID id) const
{
	DebugAssertIndex(this->meshDefs, id);
	return this->meshDefs[id];
}

const CollisionShapeDefinition &CollisionChunk::getCollisionShapeDef(CollisionShapeDefID id) const
{
	DebugAssertIndex(this->shapeDefs, id);
	return this->shapeDefs[id];
}

CollisionChunk::CollisionMeshDefID CollisionChunk::addCollisionMeshDef(CollisionMeshDefinition &&meshDef)
{
	const CollisionMeshDefID id = static_cast<CollisionMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(meshDef));
	return id;
}

CollisionChunk::CollisionShapeDefID CollisionChunk::addCollisionShapeDef(CollisionShapeDefinition &&shapeDef)
{
	const CollisionShapeDefID id = static_cast<CollisionMeshDefID>(this->shapeDefs.size());
	this->shapeDefs.emplace_back(std::move(shapeDef));
	return id;
}

CollisionChunk::CollisionMeshDefID CollisionChunk::getOrAddMeshDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelMeshDefID voxelMeshDefID)
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

CollisionChunk::CollisionShapeDefID CollisionChunk::getOrAddShapeDefIdMapping(const VoxelChunk &voxelChunk, VoxelChunk::VoxelTraitsDefID voxelTraitsDefID)
{
	CollisionChunk::CollisionShapeDefID collisionShapeDefID = -1;

	const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
	const auto iter = this->shapeMappings.find(voxelTraitsDefID);
	if (iter != this->shapeMappings.end())
	{
		collisionShapeDefID = iter->second;
	}
	else if (voxelTraitsDef.hasCollision())
	{
		double boxWidth = 1.0;
		double boxHeight = 0.0;
		double boxDepth = 1.0;
		double boxYOffset = 0.0;
		Radians boxYRotation = 0.0;

		const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			boxHeight = 1.0;
			break;
		case ArenaTypes::VoxelType::Raised:
			boxHeight = voxelTraitsDef.raised.ySize;
			boxYOffset = voxelTraitsDef.raised.yOffset;
			break;
		case ArenaTypes::VoxelType::Chasm:
			// @todo: this should just be the floor of the chasm, not an actual filling collider
			boxHeight = (voxelTraitsDef.chasm.type != ArenaTypes::ChasmType::Dry) ? ArenaChasmUtils::DEFAULT_HEIGHT : 1.0;
			break;
		case ArenaTypes::VoxelType::Diagonal:
			boxWidth = 0.1;
			boxHeight = 1.0;
			boxDepth = 0.1;
			boxYRotation = Constants::Pi / 4.0; // @todo: I guess I removed voxelTraitsDef.diagonal.type1? need it in some form here again
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
			break;
		}

		CollisionShapeDefinition collisionShapeDef;
		collisionShapeDef.initBox(boxWidth, boxHeight, boxDepth, boxYOffset, boxYRotation);
		collisionShapeDefID = this->addCollisionShapeDef(std::move(collisionShapeDef));
		this->shapeMappings.emplace(voxelTraitsDefID, collisionShapeDefID);
	}

	return collisionShapeDefID;
}
