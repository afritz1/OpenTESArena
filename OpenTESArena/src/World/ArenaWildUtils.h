#ifndef ARENA_WILD_UTILS_H
#define ARENA_WILD_UTILS_H

#include <cstdint>

#include "ArenaLevelUtils.h"
#include "LevelData.h"
#include "VoxelUtils.h"
#include "../Assets/ExeData.h"

#include "components/utilities/Buffer2D.h"

class BinaryAssetLibrary;
class LocationDefinition;
class VoxelGrid;

enum class ClimateType;
enum class WeatherType;

namespace ArenaWildUtils
{
	using WildBlockID = uint8_t; // Corresponds to WILD{...}.MIF file.

	// Chunk counts across wilderness width and height.
	constexpr int WILD_WIDTH = 64;
	constexpr int WILD_HEIGHT = WILD_WIDTH;

	// Max height of .RMD with highest MAP2 extension.
	constexpr int LEVEL_HEIGHT = 6;

	// Generates the .INF name for the wilderness given a climate and current weather.
	std::string generateInfName(ClimateType climateType, WeatherType weatherType);

	// Makes a 32-bit seed for a wilderness chunk. Intended for building names.
	uint32_t makeWildChunkSeed(int wildX, int wildY);

	// Wilderness indices for looking up WILD{...}.MIF files, generated once per world map location.
	Buffer2D<WildBlockID> generateWildernessIndices(uint32_t wildSeed,
		const ExeData::Wilderness &wildData);

	// Changes the default filler city skeleton to the one intended for the city.
	// @todo: maybe this should eventually take a bag of chunks w/ their coordinates
	// and it would search for the wild chunks that were at the city coordinates.
	void reviseWildernessCity(const LocationDefinition &locationDef, Buffer2D<uint16_t> &flor,
		Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2,
		const BinaryAssetLibrary &binaryAssetLibrary);

	// Gets the origin of a virtual 128x128 space in the wild as if the player was at the given
	// position. This space always contains 4 wild chunks.
	// @todo: when changing to chunks, probably use chunk X and Y here instead of absolute [0,4095],
	// and return the chunk coordinate that contains the origin.
	OriginalInt2 getRelativeWildOrigin(const Int2 &voxel);

	// A variation on getRelativeWildOrigin() -- determine which one is actually what we want for
	// all cases, because getRelativeWildOrigin() apparently doesn't make the automap centered.
	// Given coordinates are expected to be in original coordinate system.
	NewInt2 getCenteredWildOrigin(const NewInt2 &voxel);

	// Whether a *MENU voxel appears in the wilderness automap.
	bool menuIsDisplayedInWildAutomap(int menuIndex);
}

#endif
