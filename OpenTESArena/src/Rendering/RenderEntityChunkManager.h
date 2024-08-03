#ifndef RENDER_ENTITY_CHUNK_MANAGER_H
#define RENDER_ENTITY_CHUNK_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderEntityChunk.h"
#include "RenderEntityMeshInstance.h"
#include "RenderShaderUtils.h"
#include "../Entities/EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class EntityChunk;
class EntityChunkManager;
class EntityDefinitionLibrary;
class EntityVisibilityChunkManager;
class Renderer;
class RenderLightChunk;
class RenderLightChunkManager;
class RenderVoxelChunk;
class RenderVoxelChunkManager;
class TextureManager;
class VoxelChunkManager;

struct EntityAnimationInstance;
struct EntityVisibilityChunk;
struct RenderCamera;
struct RenderCommandBuffer;

class RenderEntityChunkManager final : public SpecializedChunkManager<RenderEntityChunk>
{
public:
	struct LoadedAnimation
	{
		EntityDefID defID;
		Buffer<ScopedObjectTextureRef> textureRefs; // Linearized based on the anim def's keyframes.

		void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
	};
private:
	// Transform buffer IDs for each entity.
	// @optimization: instead of a uniform buffer per entity, all could share one buffer, and a RecyclablePool would store the indices into that uniform buffer.
	std::unordered_map<EntityInstanceID, UniformBufferID> transformBufferIDs;

	std::vector<LoadedAnimation> anims;
	RenderEntityMeshInstance meshInst; // Shared by all entities.
	std::unordered_map<EntityPaletteIndicesInstanceID, ScopedObjectTextureRef> paletteIndicesTextureRefs;

	// All accumulated draw calls from entities each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> drawCallsCache;

	ObjectTextureID getTextureID(EntityInstanceID entityInstID, const CoordDouble2 &cameraCoordXZ, const EntityChunkManager &entityChunkManager) const;

	void loadTextures(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);
	void loadUniformBuffers(const EntityChunk &entityChunk, Renderer &renderer);

	void addDrawCall(UniformBufferID transformBufferID, int transformIndex, ObjectTextureID textureID0,
		const std::optional<ObjectTextureID> &textureID1, BufferView<const RenderLightID> lightIDs, PixelShaderType pixelShaderType,
		std::vector<RenderDrawCall> &drawCalls);
	void rebuildChunkDrawCalls(RenderEntityChunk &renderChunk, const EntityVisibilityChunk &entityVisChunk,
		const RenderLightChunk &renderLightChunk, const CoordDouble2 &cameraCoordXZ, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager);
	void rebuildDrawCallsList();
public:
	RenderEntityChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandBuffer(RenderCommandBuffer &commandBuffer) const;

	// Chunk allocating/freeing update function, called before entity resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		const CoordDouble2 &cameraCoordXZ, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager, const EntityVisibilityChunkManager &entityVisChunkManager,
		const RenderLightChunkManager &renderLightChunkManager, TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
