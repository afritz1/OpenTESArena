#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include "ChunkManager.h"
#include "../Entities/EntityManager.h"

// Instance of a level with voxels and entities. Its data is in a baked, context-sensitive format
// and depends on one or more level definitions for its population.

class MapDefinition;
class Renderer;

enum class MapType;
enum class WeatherType;

class LevelInstance
{
private:
	ChunkManager chunkManager;
	EntityManager entityManager;
	double ceilingScale;

	// @todo: probably store the table of TextureAssetReferences -> VoxelTextureID/EntityTextureID/etc. in this class.
public:
	LevelInstance();

	void init(double ceilingScale);

	ChunkManager &getChunkManager();
	const ChunkManager &getChunkManager() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	double getCeilingScale() const;

	bool trySetActive(WeatherType weatherType, bool nightLightsAreActive,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		TextureManager &textureManager, Renderer &renderer);

	void update(double dt, const ChunkInt2 &centerChunk, int activeLevelIndex,
		const MapDefinition &mapDefinition, int chunkDistance);
};

#endif
