#ifndef RENDER_CHUNK_MANAGER_H
#define RENDER_CHUNK_MANAGER_H

#include <array>
#include <optional>
#include <vector>

#include "RenderChunk.h"
#include "RenderDrawCall.h"
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
class TextureManager;
class VoxelChunkManager;

struct EntityAnimationInstance;
struct RenderCamera;

class RenderChunkManager final : public SpecializedChunkManager<RenderChunk>
{
public:
	struct LoadedVoxelTexture
	{
		TextureAsset textureAsset;
		ScopedObjectTextureRef objectTextureRef;

		void init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
	};

	// Chasm walls are multi-textured. They use the chasm animation and a separate wall texture.
	// The draw call and pixel shader need two textures in order to support chasm wall rendering.
	struct LoadedChasmFloorTextureList
	{
		ChasmDefinition::AnimationType animType;

		uint8_t paletteIndex;
		std::vector<TextureAsset> textureAssets;

		std::vector<ScopedObjectTextureRef> objectTextureRefs; // If textured, then accessed via chasm anim percent.

		LoadedChasmFloorTextureList();

		void initColor(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
		void initTextured(std::vector<TextureAsset> &&textureAssets, std::vector<ScopedObjectTextureRef> &&objectTextureRefs);

		int getTextureIndex(double chasmAnimPercent) const;
	};

	struct LoadedChasmTextureKey
	{
		ChunkInt2 chunkPos;
		VoxelChunk::ChasmDefID chasmDefID;
		int chasmFloorListIndex;
		int chasmWallIndex; // Points into voxel textures.

		void init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, int chasmFloorListIndex, int chasmWallIndex);
	};

	struct LoadedEntityAnimation
	{
		EntityDefID defID;
		Buffer<ScopedObjectTextureRef> textureRefs; // Linearized based on the anim def's keyframes.

		void init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs);
	};
private:
	// Chasm wall support - one index buffer for each face combination.
	std::array<IndexBufferID, ArenaMeshUtils::CHASM_WALL_COMBINATION_COUNT> chasmWallIndexBufferIDs;

	std::vector<LoadedVoxelTexture> voxelTextures; // Includes chasm walls.
	std::vector<LoadedChasmFloorTextureList> chasmFloorTextureLists;
	std::vector<LoadedChasmTextureKey> chasmTextureKeys; // Points into floor lists and wall textures.

	std::vector<LoadedEntityAnimation> entityAnims;
	RenderEntityMeshDefinition entityMeshDef; // Shared by all entities.
	std::unordered_map<EntityPaletteIndicesInstanceID, ScopedObjectTextureRef> entityPaletteIndicesTextureRefs;

	RenderLightID playerLightID;
	std::unordered_map<EntityInstanceID, RenderLightID> entityLightIDs; // All lights have an associated entity.

	// All accumulated draw calls from scene components each frame. This is sent to the renderer.
	std::vector<RenderDrawCall> voxelDrawCallsCache, entityDrawCallsCache;

	ObjectTextureID getVoxelTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID, double chasmAnimPercent) const;
	ObjectTextureID getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const;
	ObjectTextureID getEntityTextureID(EntityInstanceID entityInstID, const CoordDouble2 &cameraCoordXZ,
		const EntityChunkManager &entityChunkManager) const;

	void loadVoxelTextures(const VoxelChunk &voxelChunk, TextureManager &textureManager, Renderer &renderer);
	void loadVoxelMeshBuffers(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer);
	void loadVoxelChasmWall(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z);
	void loadVoxelChasmWalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk);

	void loadEntityTextures(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	void addVoxelDrawCall(const Double3 &position, const Double3 &preScaleTranslation, const Matrix4d &rotationMatrix,
		const Matrix4d &scaleMatrix, VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
		IndexBufferID indexBufferID, ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1,
		TextureSamplingType textureSamplingType0, TextureSamplingType textureSamplingType1, RenderLightingType lightingType,
		double meshLightPercent, VertexShaderType vertexShaderType, PixelShaderType pixelShaderType, double pixelShaderParam0,
		std::vector<RenderDrawCall> &drawCalls);
	void loadVoxelDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		double chasmAnimPercent, bool updateStatics, bool updateAnimating);

	// Call once per frame per chunk after all voxel chunk changes have been applied to this manager.
	// All context-sensitive data (like for chasm walls) should be available in the voxel chunk.
	void rebuildVoxelChunkDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
		double chasmAnimPercent, bool updateStatics, bool updateAnimating);
	void rebuildVoxelDrawCallsList();

	void addEntityDrawCall(const Double3 &position, const Matrix4d &rotationMatrix, const Matrix4d &scaleMatrix,
		ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1, PixelShaderType pixelShaderType,
		std::vector<RenderDrawCall> &drawCalls);
	void rebuildEntityChunkDrawCalls(RenderChunk &renderChunk, const EntityChunk &entityChunk, const CoordDouble2 &cameraCoordXZ,
		const Matrix4d &rotationMatrix, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager);
	void rebuildEntityDrawCallsList();
public:
	RenderChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	BufferView<const RenderDrawCall> getVoxelDrawCalls() const;
	BufferView<const RenderDrawCall> getEntityDrawCalls() const;

	// Chunk allocating/freeing update function, called before voxel or entity resources are updated.
	void updateActiveChunks(BufferView<const ChunkInt2> activeChunkPositions,
		BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);

	void updateVoxels(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager,
		Renderer &renderer);
	void updateEntities(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		const CoordDouble2 &cameraCoordXZ, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager, TextureManager &textureManager, Renderer &renderer);
	void updateLights(BufferView<const ChunkInt2> newChunkPositions, const CoordDouble3 &cameraCoord, double ceilingScale,
		bool isFogActive, bool nightLightsAreActive, bool playerHasLight, const EntityChunkManager &entityChunkManager, Renderer &renderer);

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
