#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <vector>

#include "CollisionMeshDefinition.h"
#include "../Math/MathUtils.h"
#include "../World/ChunkUtils.h"

#include "components/utilities/Buffer3D.h"

class CollisionChunk
{
public:
	using CollisionMeshDefID = int;
private:
	ChunkInt2 position;

	Buffer3D<CollisionMeshDefID> meshInstIDs;
	Buffer3D<bool> enabledColliders;
public:
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	static constexpr CollisionMeshDefID AIR_COLLISION_MESH_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);

	int getHeight() const;
	bool isValidVoxel(SNInt x, int y, WEInt z) const;
	const ChunkInt2 &getPosition() const;
};

#endif
