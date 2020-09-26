#ifndef CITY_LEVEL_UTILS_H
#define CITY_LEVEL_UTILS_H

#include <cstdint>
#include <vector>

#include "VoxelUtils.h"
#include "../Assets/ExeData.h"

#include "components/utilities/Buffer2D.h"

class ArenaRandom;
class MiscAssets;

namespace CityLevelUtils
{
	// Writes generated city building data into the output buffers. The buffers should already
	// be initialized with the city skeleton.
	void generateCity(uint32_t citySeed, int cityDim, WEInt gridDepth,
		const std::vector<uint8_t> &reservedBlocks, const OriginalInt2 &startPosition,
		ArenaRandom &random, const MiscAssets &miscAssets, Buffer2D<uint16_t> &dstFlor,
		Buffer2D<uint16_t> &dstMap1, Buffer2D<uint16_t> &dstMap2);

	// Iterates over the perimeter of a city map and changes palace graphics and their gates to the
	// actual ones used in-game.
	void revisePalaceGraphics(Buffer2D<uint16_t> &map1, SNInt gridWidth, WEInt gridDepth);
}

#endif
