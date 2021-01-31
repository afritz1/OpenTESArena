#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <vector>

#include "LevelData.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Math/Vector2.h"

// For instances of world data (exteriors and interiors).

class BinaryAssetLibrary;
class ExeData;
class LocationDefinition;
class MIFFile;
class ProvinceDefinition;
class TextAssetLibrary;
class TextureManager;

enum class MapType;
enum class WeatherType;

class WorldData
{
public:
	struct Interior
	{
		ArenaTypes::InteriorType interiorType;

		void init(ArenaTypes::InteriorType interiorType);
	};
private:
	std::vector<LevelData> levels;
	std::vector<NewDouble2> startPoints;
	int activeLevelIndex;

	// Map-type-specific data.
	MapType mapType;
	Interior interior;

	WorldData(MapType mapType, int activeLevelIndex);
public:
	static WorldData loadInterior(ArenaTypes::InteriorType interiorType, const MIFFile &mif, const ExeData &exeData);
	static WorldData loadDungeon(uint32_t seed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon,
		const ExeData &exeData);
	
	// Loads an exterior city skeleton and its random .MIF chunks.
	static WorldData loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		const MIFFile &mif, WeatherType weatherType, int currentDay, int starCount,
		const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager);

	// Loads wilderness for a given city on the world map.
	static WorldData loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		WeatherType weatherType, int currentDay, int starCount, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);

	MapType getMapType() const;
	int getActiveLevelIndex() const;
	int getLevelCount() const;

	LevelData &getActiveLevel();
	const LevelData &getActiveLevel() const;

	// Gets the start points within each level.
	const std::vector<NewDouble2> &getStartPoints() const;

	const WorldData::Interior &getInterior() const;

	void setActiveLevelIndex(int index);
};

#endif
