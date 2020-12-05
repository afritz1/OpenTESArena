#ifndef EXTERIOR_LEVEL_DATA_H
#define EXTERIOR_LEVEL_DATA_H

#include <cstdint>
#include <string>
#include <vector>

#include "ArenaLevelUtils.h"
#include "DistantSky.h"
#include "LevelData.h"
#include "VoxelUtils.h"
#include "../Math/Vector2.h"

#include "components/utilities/Buffer2D.h"

class LocationDefinition;
class ProvinceDefinition;
class TextAssetLibrary;

class ExteriorLevelData : public LevelData
{
private:
	DistantSky distantSky;
	ArenaLevelUtils::MenuNamesList menuNames;

	ExteriorLevelData(SNInt gridWidth, int gridHeight, WEInt gridDepth, const std::string &infName,
		const std::string &name);
public:
	ExteriorLevelData(ExteriorLevelData&&) = default;
	virtual ~ExteriorLevelData();

	// Exterior level with a pre-defined .INF file. If premade, this loads the premade city. Otherwise,
	// this loads the skeleton of the level (city walls, etc.), and fills in the rest by generating
	// the required chunks.
	static ExteriorLevelData loadCity(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, const MIFFile::Level &level, WeatherType weatherType,
		int currentDay, int starCount, const std::string &infName, SNInt gridWidth, WEInt gridDepth,
		const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager);

	// Wilderness with a pre-defined .INF file. This loads the skeleton of the wilderness
	// and fills in the rest by loading the required .RMD chunks.
	static ExteriorLevelData loadWilderness(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, WeatherType weatherType, int currentDay,
		int starCount, const std::string &infName, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);

	// Gets the mappings of voxel coordinates to *MENU display names.
	const ArenaLevelUtils::MenuNamesList &getMenuNames() const;

	// Exteriors are never outdoor dungeons (always false).
	virtual bool isOutdoorDungeon() const override;

	// Calls the base level data method then does some exterior-specific work.
	virtual void setActive(bool nightLightsAreActive, const WorldData &worldData,
		const ProvinceDefinition &provinceDef, const LocationDefinition &locationDef,
		const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, Random &random, CitizenManager &citizenManager,
		TextureManager &textureManager, TextureInstanceManager &textureInstManager,
		Renderer &renderer) override;

	// Updates data exclusive to exterior level data (such as animated distant land).
	virtual void tick(Game &game, double dt) override;
};

#endif
