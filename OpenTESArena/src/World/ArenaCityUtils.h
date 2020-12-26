#ifndef ARENA_CITY_UTILS_H
#define ARENA_CITY_UTILS_H

#include <cstdint>
#include <vector>

#include "ArenaLevelUtils.h"
#include "LevelData.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/MIFFile.h"

#include "components/dos/DOSUtils.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView2D.h"

class ArenaRandom;
class BinaryAssetLibrary;
class LocationDefinition;
class ProvinceDefinition;
class TextAssetLibrary;
class VoxelGrid;

enum class ClimateType;
enum class WeatherType;

// @todo: these should probably all be BufferView2D instead of Buffer2D

namespace ArenaCityUtils
{
	// Generates the .INF name for a city given a climate and current weather.
	DOSUtils::FilenameBuffer generateInfName(ClimateType climateType, WeatherType weatherType);

	// Writes the barebones city layout (just ground and walls).
	void writeSkeleton(const MIFFile::Level &level, BufferView2D<ArenaTypes::VoxelID> &dstFlor,
		BufferView2D<ArenaTypes::VoxelID> &dstMap1, BufferView2D<ArenaTypes::VoxelID> &dstMap2);

	// Writes generated city building data into the output buffers. The buffers should already
	// be initialized with the city skeleton.
	void generateCity(uint32_t citySeed, int cityDim, WEInt gridDepth,
		const BufferView<const uint8_t> &reservedBlocks, const OriginalInt2 &startPosition,
		ArenaRandom &random, const BinaryAssetLibrary &binaryAssetLibrary,
		Buffer2D<ArenaTypes::VoxelID> &dstFlor, Buffer2D<ArenaTypes::VoxelID> &dstMap1,
		Buffer2D<ArenaTypes::VoxelID> &dstMap2);

	// Iterates over the perimeter of a city map and changes palace graphics and their gates to the
	// actual ones used in-game.
	// @todo: this should use Arena dimensions (from MAP1?), not modern dimensions
	void revisePalaceGraphics(Buffer2D<ArenaTypes::VoxelID> &map1, SNInt gridWidth, WEInt gridDepth);

	// Creates mappings of *MENU voxel coordinates to *MENU names. Call this after voxels have
	// been loaded into the voxel grid so that voxel bits don't have to be decoded twice.
	ArenaLevelUtils::MenuNamesList generateBuildingNames(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, ArenaRandom &random, const VoxelGrid &voxelGrid,
		const std::vector<LevelData::Transition> &transitions, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary);
}

#endif
