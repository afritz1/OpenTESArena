#ifndef ARENA_CITY_UTILS_H
#define ARENA_CITY_UTILS_H

#include <cstdint>
#include <vector>

#include "ArenaLevelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/MIFFile.h"
#include "../Voxels/VoxelUtils.h"
#include "../Weather/WeatherDefinition.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/Span2D.h"

class ArenaRandom;
class BinaryAssetLibrary;
class LocationDefinition;
class ProvinceDefinition;
class TextAssetLibrary;

// @todo: these should probably all be BufferView2D instead of Buffer2D
namespace ArenaCityUtils
{
	// Max height of .MIF with highest MAP2 extension.
	constexpr int LEVEL_HEIGHT = 6;

	// Generates the .INF name for a city given a climate and current weather.
	std::string generateInfName(ArenaClimateType climateType, WeatherType weatherType);

	// Writes the barebones city layout (just ground and walls).
	void writeSkeleton(const MIFLevel &level, Span2D<ArenaVoxelID> &dstFlor,
		Span2D<ArenaVoxelID> &dstMap1, Span2D<ArenaVoxelID> &dstMap2);

	// Writes generated city building data into the output buffers. The buffers should already
	// be initialized with the city skeleton.
	void generateCity(uint32_t citySeed, int cityDim, WEInt gridDepth, Span<const uint8_t> reservedBlocks,
		const OriginalInt2 &startPosition, ArenaRandom &random, const BinaryAssetLibrary &binaryAssetLibrary,
		Buffer2D<ArenaVoxelID> &dstFlor, Buffer2D<ArenaVoxelID> &dstMap1,
		Buffer2D<ArenaVoxelID> &dstMap2);

	// Iterates over the perimeter of a city map and changes palace graphics and their gates to the
	// actual ones used in-game.
	// @todo: this should use Arena dimensions (from MAP1?), not modern dimensions
	void revisePalaceGraphics(Buffer2D<ArenaVoxelID> &map1, SNInt gridWidth, WEInt gridDepth);
}

#endif
