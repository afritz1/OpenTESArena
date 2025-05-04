#ifndef ENTITY_VISIBILITY_CHUNK_H
#define ENTITY_VISIBILITY_CHUNK_H

#include <vector>

#include "EntityInstance.h"
#include "../Math/BoundingBox.h"
#include "../World/Chunk.h"

class EntityChunk;
class EntityChunkManager;

struct RenderCamera;

struct VisibleEntityEntry
{
	EntityInstanceID id;
	WorldDouble3 position;
};

struct EntityVisibilityChunk final : public Chunk
{
	BoundingBox3D bbox; // Expands to include all entities in this chunk.
	std::vector<BoundingBox3D> entityWorldBBoxCache; // Only for reusing bounding boxes inside of update().
	std::vector<VisibleEntityEntry> visibleEntityEntries;

	void init(const ChunkInt2 &position, int height);
	void update(const RenderCamera &camera, double ceilingScale, const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager);
	void clear();
};

#endif
