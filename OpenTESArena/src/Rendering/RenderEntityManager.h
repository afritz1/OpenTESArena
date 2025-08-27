#ifndef RENDER_ENTITY_MANAGER_H
#define RENDER_ENTITY_MANAGER_H

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

	RenderEntityLoadedAnimation();

	void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
};

struct RenderEntityPaletteIndicesEntry
{
	EntityPaletteIndicesInstanceID paletteIndicesInstanceID;
	ObjectTextureID textureID; // Palette indices as renderer texture.
	Buffer<RenderMaterialID> materialIDs; // Linearized animation material IDs.

	RenderEntityPaletteIndicesEntry();
};

class RenderEntityManager
{
private:
	std::vector<RenderEntityLoadedAnimation> anims;
	RenderMeshInstance meshInst; // Shared by all entities.
	std::vector<RenderEntityPaletteIndicesEntry> paletteIndicesEntries; // Unique to each citizen, contains allocated palette texture and material IDs.

	std::vector<RenderMaterial> materials; // Loaded for every non-citizen animation.

	// All accumulated draw calls from entities each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;
	std::vector<RenderDrawCall> puddleSecondPassDrawCallsCache;

	void loadMaterialsForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);
public:
	RenderEntityManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	// For entities not from the level itself (i.e. VFX).
	void loadMaterialsForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer);

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
