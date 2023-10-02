#ifndef RENDER_ENTITY_CHUNK_MANAGER_H
#define RENDER_ENTITY_CHUNK_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderEntityChunk.h"
#include "RenderEntityMeshDefinition.h"
#include "RenderShaderUtils.h"
#include "../Entities/EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class EntityChunk;
class EntityChunkManager;
class EntityDefinitionLibrary;
class Renderer;
class RenderVoxelChunk;
class RenderVoxelChunkManager;
class TextureManager;
class VoxelChunkManager;

struct EntityAnimationInstance;
struct RenderCamera;

class RenderEntityChunkManager final : public SpecializedChunkManager<RenderEntityChunk>
{
public:
	struct LoadedEntityAnimation
	{
		EntityDefID defID;
		Buffer<ScopedObjectTextureRef> textureRefs; // Linearized based on the anim def's keyframes.

		void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
	};
private:
	// Transform buffer IDs for each entity.
	// @optimization: instead of a uniform buffer per entity, all could share one buffer, and a RecyclablePool would store the indices into that uniform buffer.
	std::unordered_map<EntityInstanceID, UniformBufferID> entityTransformBufferIDs;

	std::vector<LoadedEntityAnimation> entityAnims;
	RenderEntityMeshDefinition entityMeshDef; // Shared by all entities.
	std::unordered_map<EntityPaletteIndicesInstanceID, ScopedObjectTextureRef> entityPaletteIndicesTextureRefs;

	// All accumulated draw calls from scene components each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> entityDrawCallsCache;

	ObjectTextureID getEntityTextureID(EntityInstanceID entityInstID, const CoordDouble2 &cameraCoordXZ,
		const EntityChunkManager &entityChunkManager) const;

	void loadEntityTextures(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
		TextureManager &textureManager, Renderer &renderer);
	void loadEntityUniformBuffers(const EntityChunk &entityChunk, Renderer &renderer);

	void addEntityDrawCall(const Double3 &position, UniformBufferID transformBufferID, int transformIndex,
		ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1, BufferView<const RenderLightID> lightIDs,
		PixelShaderType pixelShaderType, std::vector<RenderDrawCall> &drawCalls);
	void rebuildEntityChunkDrawCalls(RenderEntityChunk &renderChunk, const EntityChunk &entityChunk, const RenderVoxelChunk &renderVoxelChunk,
		const CoordDouble2 &cameraCoordXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager);
	void rebuildEntityDrawCallsList();
public:
	RenderEntityChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	BufferView<const RenderDrawCall> getEntityDrawCalls() const;

	// Chunk allocating/freeing update function, called before voxel or entity resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void updateEntities(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		const CoordDouble2 &cameraCoordXZ, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager, const RenderVoxelChunkManager &renderVoxelChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
