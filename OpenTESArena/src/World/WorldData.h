#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <string>
#include <vector>

#include "LevelData.h"
#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"

// This class stores data regarding elements in the game world. It should be constructible
// from a pair of .MIF and .INF files.

class INFFile;
class MIFFile;
class Renderer;
class TextureManager;

class WorldData
{	
private:
	std::vector<LevelData> levels;
	std::vector<Double2> startPoints;
	EntityManager entityManager;
	int currentLevel;
public:
	WorldData(const MIFFile &mif, const INFFile &inf);
	WorldData(VoxelGrid &&voxelGrid, EntityManager &&entityManager); // Used with test city.
	WorldData(WorldData &&worldData) = default;
	~WorldData();

	WorldData &operator=(WorldData &&worldData) = default;

	int getCurrentLevel() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	const std::vector<Double2> &getStartPoints() const;
	std::vector<LevelData> &getLevels();
	const std::vector<LevelData> &getLevels() const;

	void switchToLevel(int levelIndex, TextureManager &textureManager,
		Renderer &renderer);
};

#endif
