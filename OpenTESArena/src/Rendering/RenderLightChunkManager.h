#ifndef RENDER_LIGHT_CHUNK_MANAGER_H
#define RENDER_LIGHT_CHUNK_MANAGER_H

#include <unordered_map>
#include <vector>

#include "RenderLightChunk.h"
#include "RenderShaderUtils.h"
#include "../Entities/EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class Renderer;
class EntityChunkManager;
class VoxelChunkManager;

class RenderLightChunkManager final : public SpecializedChunkManager<RenderLightChunk>
{
public:
	struct Light
	{
		RenderLightID lightID;
		WorldDouble3 point;
		WorldDouble3 minPoint, maxPoint; // Bounding box, updated when the light moves.
		std::vector<WorldInt3> voxels, addedVoxels, removedVoxels; // Current, newly-touched, and no-longer-touched voxels this frame.

		double startRadius, endRadius;
		bool enabled; // Enabled lights influence light ID lists and can be used in draw calls.

		Light();

		void init(RenderLightID lightID, const WorldDouble3 &point, double startRadius, double endRadius, bool enabled, double ceilingScale, int chunkHeight);
		void update(const WorldDouble3 &point, double startRadius, double endRadius, double ceilingScale, int chunkHeight);
		void clear();
	};
private:
	Light playerLight;
	std::unordered_map<EntityInstanceID, Light> entityLights; // All lights have an associated entity.
	bool isSceneChanged; // Supports player light position changes between interiors.

	void registerLightToVoxels(const Light &light, Span<const WorldInt3> voxels, double ceilingScale);
	void unregisterLightFromVoxels(const Light &light, Span<const WorldInt3> voxels);
public:
	RenderLightChunkManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	void loadScene();

	// Chunk allocating/freeing update function, called before light resources are updated.
	void updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager, Renderer &renderer);
	
	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
		const CoordDouble3 &cameraCoord, double ceilingScale, bool isFogActive, bool nightLightsAreActive, bool playerHasLight,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, Renderer &renderer);

	void setNightLightsActive(bool enabled, double ceilingScale, const EntityChunkManager &entityChunkManager);

	// End of frame clean-up.
	void cleanUp();

	// Clears all allocated rendering resources.
	void unloadScene(Renderer &renderer);
};

#endif
