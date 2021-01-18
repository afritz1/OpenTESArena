#ifndef MAP_INSTANCE_H
#define MAP_INSTANCE_H

#include "LevelInstance.h"
#include "SkyInstance.h"

#include "components/utilities/Buffer.h"

// Contains instance data for the associated map definition. This is the current state of voxels,
// entities, and sky for every level instance in the map.

class MapDefinition;
class TextureManager;

class MapInstance
{
private:
	Buffer<LevelInstance> levels;
	Buffer<SkyInstance> skies;
	int activeLevelIndex;
	int activeSkyIndex;

	void initInterior(const MapDefinition &mapDefinition, TextureManager &textureManager);
	void initCity(const MapDefinition &mapDefinition, TextureManager &textureManager);
	void initWild(const MapDefinition &mapDefinition, TextureManager &textureManager);
public:
	MapInstance();

	void init(const MapDefinition &mapDefinition, TextureManager &textureManager);

	int getLevelCount() const;
	LevelInstance &getLevel(int index);
	const LevelInstance &getLevel(int index) const;
	LevelInstance &getActiveLevel();
	const LevelInstance &getActiveLevel() const;
	
	int getSkyCount() const;
	SkyInstance &getSky(int index);
	const SkyInstance &getSky(int index) const;
	SkyInstance &getActiveSky();
	const SkyInstance &getActiveSky() const;

	void setActiveLevelIndex(int levelIndex);

	void update(double dt, const ChunkInt2 &centerChunk, const MapDefinition &mapDefinition,
		double latitude, double daytimePercent, int chunkDistance);
};

#endif
