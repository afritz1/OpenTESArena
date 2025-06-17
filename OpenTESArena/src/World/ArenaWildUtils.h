#ifndef ARENA_WILD_UTILS_H
#define ARENA_WILD_UTILS_H

#include <cstdint>

#include "ArenaLevelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../Voxels/VoxelUtils.h"
#include "../Weather/WeatherDefinition.h"
#include "../WorldMap/LocationDefinition.h"

#include "components/utilities/Buffer2D.h"

class BinaryAssetLibrary;

namespace ArenaWildUtils
{
	using WildBlockID = uint8_t; // Corresponds to WILD{...}.MIF file.

	// Chunk counts across wilderness width and height.
	constexpr int WILD_WIDTH = 64;
	constexpr int WILD_HEIGHT = WILD_WIDTH;

	// Max height of .RMD with highest MAP2 extension.
	constexpr int LEVEL_HEIGHT = 6;

	// Chunk offsets of the city from the wilderness origin.
	constexpr int CITY_ORIGIN_CHUNK_X = 31;
	constexpr int CITY_ORIGIN_CHUNK_Z = CITY_ORIGIN_CHUNK_X;

	// Number of dungeon chunks (32x32) wide and tall wild dungeons are.
	constexpr int WILD_DUNGEON_WIDTH_CHUNKS = 2;
	constexpr int WILD_DUNGEON_HEIGHT_CHUNKS = WILD_DUNGEON_WIDTH_CHUNKS;
	
	// .INF flat index for determining if a flat is a transition to a wild dungeon.
	constexpr ArenaFlatIndex WILD_DEN_FLAT_INDEX = 37;

	// Generates the .INF name for the wilderness given a climate and current weather.
	std::string generateInfName(ArenaClimateType climateType, WeatherType weatherType);

	// Makes a 32-bit seed for a wilderness chunk. Intended for building names.
	uint32_t makeWildChunkSeed(int wildX, int wildY);

	// Wilderness indices for looking up WILD{...}.MIF files, generated once per world map location.
	Buffer2D<WildBlockID> generateWildernessIndices(uint32_t wildSeed, const ExeDataWilderness &wildData);

	// Returns whether the given WILD{...}.MIF block ID is for a city block.
	bool isWildCityBlock(ArenaWildUtils::WildBlockID wildBlockID);

	// Changes the default filler city skeleton to the one intended for the city.
	void reviseWildCityBlock(ArenaWildUtils::WildBlockID wildBlockID, Span2D<ArenaVoxelID> &flor,
		Span2D<ArenaVoxelID> &map1, Span2D<ArenaVoxelID> &map2,
		const LocationCityDefinition &cityDef, const BinaryAssetLibrary &binaryAssetLibrary);

	// Gets the origin of a virtual 128x128 space in the wild as if the player was at the given
	// position. This space always contains 4 wild chunks.
	// @todo: when changing to chunks, probably use chunk X and Y here instead of absolute [0,4095],
	// and return the chunk coordinate that contains the origin.
	OriginalInt2 getRelativeWildOrigin(const Int2 &voxel);

	// A variation on getRelativeWildOrigin() -- determine which one is actually what we want for
	// all cases, because getRelativeWildOrigin() apparently doesn't make the automap centered.
	// Given coordinates are expected to be in original coordinate system.
	WorldInt2 getCenteredWildOrigin(const WorldInt2 &voxel);

	// Whether a *MENU voxel appears in the wilderness automap.
	bool menuIsDisplayedInWildAutomap(int menuIndex);
}

#endif
