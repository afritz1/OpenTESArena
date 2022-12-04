#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "../Assets/ArenaTypes.h"
#include "../Entities/CitizenUtils.h"
#include "../Entities/EntityGeneration.h"
#include "../Entities/EntityManager.h"
#include "../Voxels/VoxelChunkManager.h"

#include "components/utilities/BufferView.h"

// Instance of a level with voxels and entities. Its data is in a baked, context-sensitive format
// and depends on one or more level definitions for its population.

class AudioManager;
class EntityDefinitionLibrary;
class MapDefinition;
class RenderChunkManager;
class Renderer;
class WeatherDefinition;

enum class MapType;

class LevelInstance
{
private:
	// @todo: problem to consider here:
	// - why do we load voxel and entity textures before they are instantiated in the world?
	// - we make the assumption that "a level has voxel and entity textures" but that is decoupled from actual voxel and entity instances.
	// - feels like all voxel/entity/sky/particle object texture loading should be on demand...? Might simplify enemy spawning code.

	VoxelChunkManager voxelChunkManager;
	EntityManager entityManager;

	// Texture handles for the active game world palette and light table.
	ScopedObjectTextureRef paletteTextureRef, lightTableTextureRef;

	double ceilingScale;
public:
	LevelInstance();

	void init(double ceilingScale);

	VoxelChunkManager &getVoxelChunkManager();
	const VoxelChunkManager &getVoxelChunkManager() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	ObjectTextureID getPaletteTextureID() const;
	ObjectTextureID getLightTableTextureID() const;
	double getCeilingScale() const;

	bool trySetActive(RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer);

	void update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		int chunkDistance, double chasmAnimPercent, RenderChunkManager &renderChunkManager, TextureManager &textureManager,
		AudioManager &audioManager, Renderer &renderer);

	void cleanUp();
};

#endif
