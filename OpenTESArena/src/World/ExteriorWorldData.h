#ifndef EXTERIOR_WORLD_DATA_H
#define EXTERIOR_WORLD_DATA_H

#include <cstdint>
#include <vector>

#include "ExteriorLevelData.h"
#include "LevelData.h"
#include "WorldData.h"
#include "../Assets/INFFile.h"
#include "../Math/Vector2.h"

class BinaryAssetLibrary;
class ExeData;
class LocationDefinition;
class MIFFile;
class ProvinceDefinition;
class TextAssetLibrary;

enum class ClimateType;
enum class WeatherType;

class ExteriorWorldData : public WorldData
{
private:
	ExteriorLevelData levelData;
	bool isCity; // True if city, false if wilderness.

	ExteriorWorldData(ExteriorLevelData &&levelData, bool isCity);
public:
	ExteriorWorldData(ExteriorWorldData&&) = default;
	virtual ~ExteriorWorldData();

	// Loads an exterior city skeleton and its random .MIF chunks.
	static ExteriorWorldData loadCity(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, const MIFFile &mif, WeatherType weatherType,
		int currentDay, int starCount, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager);

	// Loads wilderness for a given city on the world map.
	static ExteriorWorldData loadWilderness(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, WeatherType weatherType, int currentDay,
		int starCount, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);

	virtual MapType getMapType() const override;

	virtual LevelData &getActiveLevel() override;
	virtual const LevelData &getActiveLevel() const override;
};

#endif
