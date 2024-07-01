#ifndef ENTITY_VISIBILITY_CHUNK_MANAGER_H
#define ENTITY_VISIBILITY_CHUNK_MANAGER_H

#include "EntityVisibilityChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

class EntityChunkManager;
class VoxelChunkManager;

struct RenderCamera;

class EntityVisibilityChunkManager final : public SpecializedChunkManager<EntityVisibilityChunk>
{
public:
	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		BufferView<const ChunkInt2> freedChunkPositions, const RenderCamera &camera, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager);
};

#endif
