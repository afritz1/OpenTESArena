#ifndef EXTERIOR_LEVEL_DATA_H
#define EXTERIOR_LEVEL_DATA_H

#include <cstdint>
#include <string>
#include <vector>

#include "DistantSky.h"
#include "LevelData.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Vector2.h"

class ExteriorLevelData : public LevelData
{
private:
	DistantSky distantSky;

	// Mappings of voxel coordinates to *MENU display names.
	std::vector<std::pair<Int2, std::string>> menuNames;

	ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
		const std::string &name);

	// Creates mappings of *MENU voxel coordinates to *MENU names. Call this after voxels have
	// been loaded into the voxel grid so that voxel bits don't have to be decoded twice.
	void generateBuildingNames(int localCityID, int provinceID, uint32_t citySeed,
		ArenaRandom &random, bool isCoastal, bool isCity, int gridWidth, int gridDepth,
		const MiscAssets &miscAssets);

	// This algorithm runs over the perimeter of a city map and changes palace graphics and
	// their gates to the actual ones used in-game.
	static void revisePalaceGraphics(std::vector<uint16_t> &map1, int gridWidth, int gridDepth);
public:
	ExteriorLevelData(ExteriorLevelData&&) = default;
	virtual ~ExteriorLevelData();

	// Premade exterior level with a pre-defined .INF file. Only used by center province.
	static ExteriorLevelData loadPremadeCity(const MIFFile::Level &level, WeatherType weatherType,
		int currentDay, const std::string &infName, int gridWidth, int gridDepth,
		const MiscAssets &miscAssets, TextureManager &textureManager);

	// Exterior level with a pre-defined .INF file (for randomly generated cities). This loads
	// the skeleton of the level (city walls, etc.), and fills in the rest by loading the
	// required .MIF chunks.
	static ExteriorLevelData loadCity(const MIFFile::Level &level, int localCityID,
		int provinceID, WeatherType weatherType, int currentDay, int cityDim, bool isCoastal,
		const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
		const std::string &infName, int gridWidth, int gridDepth, const MiscAssets &miscAssets,
		TextureManager &textureManager);

	// Wilderness with a pre-defined .INF file. This loads the skeleton of the wilderness
	// and fills in the rest by loading the required .RMD chunks.
	static ExteriorLevelData loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL,
		WeatherType weatherType, int currentDay, const std::string &infName,
		const MiscAssets &miscAssets, TextureManager &textureManager);

	// Gets the mappings of voxel coordinates to *MENU display names.
	const std::vector<std::pair<Int2, std::string>> &getMenuNames() const;

	// Exteriors are never outdoor dungeons (always false).
	virtual bool isOutdoorDungeon() const override;

	// Calls the base level data method then does some exterior-specific work.
	virtual void setActive(TextureManager &textureManager, Renderer &renderer) override;

	// Updates data exclusive to exterior level data (such as animated distant land).
	virtual void tick(double dt) override;
};

#endif
