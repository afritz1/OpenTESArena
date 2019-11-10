#ifndef EXTERIOR_LEVEL_DATA_H
#define EXTERIOR_LEVEL_DATA_H

#include <cstdint>
#include <string>
#include <vector>

#include "DistantSky.h"
#include "LevelData.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Vector2.h"

#include "components/utilities/Buffer2D.h"

class ExteriorLevelData : public LevelData
{
private:
	DistantSky distantSky;

	// Mappings of voxel coordinates to *MENU display names.
	std::vector<std::pair<Int2, std::string>> menuNames;

	ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
		const std::string &name);

	// Writes city building data into the output buffers. The buffers should already be
	// initialized with the city skeleton.
	static void generateCity(int localCityID, int provinceID, int cityDim, int gridDepth,
		const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition, uint32_t citySeed,
		ArenaRandom &random, const MiscAssets &miscAssets, std::vector<uint16_t> &dstFlor,
		std::vector<uint16_t> &dstMap1, std::vector<uint16_t> &dstMap2);

	// Creates mappings of *MENU voxel coordinates to *MENU names. Call this after voxels have
	// been loaded into the voxel grid so that voxel bits don't have to be decoded twice.
	void generateBuildingNames(int localCityID, int provinceID, uint32_t citySeed,
		ArenaRandom &random, bool isCoastal, bool isCity, int gridWidth, int gridDepth,
		const MiscAssets &miscAssets);

	// Creates mappings of wilderness *MENU voxel coordinates to *MENU names.
	void generateWildChunkBuildingNames(int localCityID, int provinceID,
		const MiscAssets &miscAssets);

	// This algorithm runs over the perimeter of a city map and changes palace graphics and
	// their gates to the actual ones used in-game.
	static void revisePalaceGraphics(std::vector<uint16_t> &map1, int gridWidth, int gridDepth);

	// Wilderness indices for looking up WILD{...}.MIF files, generated once per world map location.
	static Buffer2D<uint8_t> generateWildernessIndices(uint32_t wildSeed,
		const ExeData::Wilderness &wildData);

	// Changes the default filler city skeleton to the one intended for the city.
	static void reviseWildernessCity(int localCityID, int provinceID, Buffer2D<uint16_t> &flor,
		Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2, const MiscAssets &miscAssets);
public:
	ExteriorLevelData(ExteriorLevelData&&) = default;
	virtual ~ExteriorLevelData();

	// Gets the origin of a virtual 128x128 space in the wild as if the player was at the given
	// position. This space always contains 4 wild chunks.
	// @todo: when changing to chunks, probably use chunk X and Y here instead of absolute [0,4095],
	// and return the chunk coordinate that contains the origin.
	static Int2 getRelativeWildOrigin(const Int2 &voxel);

	// A variation on getRelativeWildOrigin() -- determine which one is actually what we want for
	// all cases, because getRelativeWildOrigin() apparently doesn't make the automap centered.
	// Given coordinates are expected to be in original coordinate system.
	static Int2 getCenteredWildOrigin(const Int2 &voxel);

	// Premade exterior level with a pre-defined .INF file. Only used by center province.
	static ExteriorLevelData loadPremadeCity(const MIFFile::Level &level, WeatherType weatherType,
		int currentDay, int starCount, const std::string &infName, int gridWidth, int gridDepth,
		const MiscAssets &miscAssets, TextureManager &textureManager);

	// Exterior level with a pre-defined .INF file (for randomly generated cities). This loads
	// the skeleton of the level (city walls, etc.), and fills in the rest by loading the
	// required .MIF chunks.
	static ExteriorLevelData loadCity(const MIFFile::Level &level, int localCityID,
		int provinceID, WeatherType weatherType, int currentDay, int starCount, int cityDim,
		bool isCoastal, const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
		const std::string &infName, int gridWidth, int gridDepth, const MiscAssets &miscAssets,
		TextureManager &textureManager);

	// Wilderness with a pre-defined .INF file. This loads the skeleton of the wilderness
	// and fills in the rest by loading the required .RMD chunks.
	static ExteriorLevelData loadWilderness(int localCityID, int provinceID,
		WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
		const MiscAssets &miscAssets, TextureManager &textureManager);

	// Gets the mappings of voxel coordinates to *MENU display names.
	const std::vector<std::pair<Int2, std::string>> &getMenuNames() const;

	// Exteriors are never outdoor dungeons (always false).
	virtual bool isOutdoorDungeon() const override;

	// Calls the base level data method then does some exterior-specific work.
	virtual void setActive(const ExeData &exeData, TextureManager &textureManager,
		Renderer &renderer) override;

	// Updates data exclusive to exterior level data (such as animated distant land).
	virtual void tick(Game &game, double dt) override;
};

#endif
