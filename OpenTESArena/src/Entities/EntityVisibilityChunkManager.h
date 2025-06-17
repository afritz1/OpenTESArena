#ifndef ENTITY_VISIBILITY_CHUNK_MANAGER_H
#define ENTITY_VISIBILITY_CHUNK_MANAGER_H

#include "EntityVisibilityChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class EntityChunkManager;
class VoxelChunkManager;

struct RenderCamera;

class EntityVisibilityChunkManager final : public SpecializedChunkManager<EntityVisibilityChunk>
{
public:
	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
		Span<const ChunkInt2> freedChunkPositions, const RenderCamera &camera, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager);
};

#endif
