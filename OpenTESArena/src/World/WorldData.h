#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <cstdint>
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

enum class ClimateType;
enum class LocationType;
enum class WeatherType;
enum class WorldType;

class WorldData
{	
private:
	// Private constructor for static WorldData load methods.
	WorldData();

	std::vector<LevelData> levels;
	std::vector<Double2> startPoints;
	EntityManager entityManager;
	std::string mifName;
	WorldType worldType;
	int currentLevel;

	// Generates the .INF name for a city given a climate and current weather.
	static std::string generateCityInfName(ClimateType climateType, WeatherType weatherType);

	// Generates the .INF name for the wilderness given a climate and current weather.
	static std::string generateWildernessInfName(ClimateType climateType, WeatherType weatherType);
public:
	// Used with test city.
	WorldData(VoxelGrid &&voxelGrid, EntityManager &&entityManager);
	WorldData(WorldData &&worldData) = default;
	~WorldData();

	WorldData &operator=(WorldData &&worldData) = default;

	// Returns whether the given ID is for a city-state, town, or village.
	static LocationType getLocationTypeFromID(int cityID);

	// Loads all levels of an interior .MIF file.
	static WorldData loadInterior(const MIFFile &mif);

	// Loads a premade exterior city (only used by center province).
	static WorldData loadPremadeCity(const MIFFile &mif, ClimateType climateType,
		WeatherType weatherType);

	// Loads an exterior city skeleton and its random .MIF chunks.
	static WorldData loadCity(int cityID, const MIFFile &mif, int cityX, int cityY,
		int cityDim, const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
		LocationType locationType, WeatherType weatherType);

	// Loads some wilderness blocks.
	static WorldData loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL,
		ClimateType climateType, WeatherType weatherType);

	int getCurrentLevel() const;
	WorldType getWorldType() const;
	const std::string &getMifName() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	const std::vector<Double2> &getStartPoints() const;
	std::vector<LevelData> &getLevels();
	const std::vector<LevelData> &getLevels() const;

	// Refreshes texture manager and renderer state using the selected level's data.
	void setLevelActive(int levelIndex, TextureManager &textureManager,
		Renderer &renderer);
};

#endif
