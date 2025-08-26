#ifndef RENDER_ENTITY_MANAGER_H
#define RENDER_ENTITY_MANAGER_H

#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderMaterialUtils.h"
#include "RenderMeshInstance.h"
#include "RenderShaderUtils.h"
#include "../Entities/EntityInstance.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

class EntityChunkManager;
class EntityVisibilityChunkManager;
class Renderer;
class TextureManager;

struct EntityChunk;
struct RenderCommandList;

struct RenderEntityLoadedAnimation
{
	EntityDefID defID;
	Buffer<ScopedObjectTextureRef> textureRefs; // Linearized based on the anim def's keyframes.

	void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
};

class RenderEntityManager
{
private:
	std::vector<RenderEntityLoadedAnimation> anims;
	RenderMeshInstance meshInst; // Shared by all entities.
	std::unordered_map<EntityPaletteIndicesInstanceID, ScopedObjectTextureRef> paletteIndicesTextureRefs;

	std::vector<RenderMaterial> materials;

	// All accumulated draw calls from entities each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;
	std::vector<RenderDrawCall> puddleSecondPassDrawCallsCache;

	ObjectTextureID getTextureID(EntityInstanceID entityInstID, const WorldDouble3 &cameraPosition, const EntityChunkManager &entityChunkManager) const;

	void loadTexturesForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);
public:
	RenderEntityManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	// For entities not from the level itself (i.e. VFX).
	void loadTexturesForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer);

	void populateCommandList(RenderCommandList &commandList) const;

	void loadScene(TextureManager &textureManager, Renderer &renderer);

	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions, const WorldDouble3 &cameraPosition,
		const VoxelDouble2 &cameraDirXZ, double ceilingScale, const EntityChunkManager &entityChunkManager, const EntityVisibilityChunkManager &entityVisChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void endFrame();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
