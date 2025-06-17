#ifndef RENDER_ENTITY_CHUNK_MANAGER_H
#define RENDER_ENTITY_CHUNK_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderEntityChunk.h"
#include "RenderMeshInstance.h"
#include "RenderShaderUtils.h"
#include "../Entities/EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

class EntityChunkManager;
class EntityDefinitionLibrary;
class EntityVisibilityChunkManager;
class Renderer;
class RenderLightChunkManager;
class RenderVoxelChunkManager;
class TextureManager;
class VoxelChunkManager;

struct EntityAnimationInstance;
struct EntityChunk;
struct EntityVisibilityChunk;
struct RenderCamera;
struct RenderCommandBuffer;
struct RenderLightChunk;
struct RenderVoxelChunk;

struct RenderEntityLoadedAnimation
{
	EntityDefID defID;
	Buffer<ScopedObjectTextureRef> textureRefs; // Linearized based on the anim def's keyframes.

	void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
};

class RenderEntityChunkManager final : public SpecializedChunkManager<RenderEntityChunk>
{
private:
	std::vector<RenderEntityLoadedAnimation> anims;
	RenderEntityMeshInstance meshInst; // Shared by all entities.
	std::unordered_map<EntityPaletteIndicesInstanceID, ScopedObjectTextureRef> paletteIndicesTextureRefs;

	// All accumulated draw calls from entities each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;

	ObjectTextureID getTextureID(EntityInstanceID entityInstID, const WorldDouble3 &cameraPosition, const EntityChunkManager &entityChunkManager) const;

	void loadTexturesForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);

	void addDrawCall(UniformBufferID transformBufferID, int transformIndex, ObjectTextureID textureID0,
		const std::optional<ObjectTextureID> &textureID1, Span<const RenderLightID> lightIDs, PixelShaderType pixelShaderType,
		std::vector<RenderDrawCall> &drawCalls);
	void rebuildChunkDrawCalls(RenderEntityChunk &renderChunk, const EntityVisibilityChunk &entityVisChunk,
		const RenderLightChunk &renderLightChunk, const WorldDouble3 &cameraPosition, double ceilingScale,
		const EntityChunkManager &entityChunkManager);
	void rebuildDrawCallsList();
public:
	RenderEntityChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	// For entities not from the level itself (i.e. VFX).
	void loadTexturesForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer);

	void populateCommandBuffer(RenderCommandBuffer &commandBuffer) const;

	void loadScene(TextureManager &textureManager, Renderer &renderer);

	// Chunk allocating/freeing update function, called before entity resources are updated.
	void updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
		const WorldDouble3 &cameraPosition, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager, const EntityVisibilityChunkManager &entityVisChunkManager,
		const RenderLightChunkManager &renderLightChunkManager, TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
