#ifndef ARENA_LEVEL_UTILS_H
#define ARENA_LEVEL_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

#include "LocationDefinition.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/utilities/BufferView2D.h"

class ArenaRandom;
class ExeData;

enum class MapType;

// Various functions for working with Arena level data, shared between world types.

namespace ArenaLevelUtils
{
	// The distance in voxels that doors will auto-close when the player is far enough away.
	constexpr double DOOR_CLOSE_DISTANCE = 3.0; // @todo: probably make this a multiple/fraction of ARENA_UNITS

	// Display names for *MENU transition voxels in cities and the wilderness.
	using MenuNamesList = std::vector<std::pair<NewInt2, std::string>>;

	// Converts an Arena ceiling height from "centimeters" to modern coordinates (1.0 per voxel).
	double convertArenaCeilingHeight(int ceilingHeight);

	// Gets the number of voxels a MAP2 voxel occupies vertically (at least 1).
	int getMap2VoxelHeight(ArenaTypes::VoxelID map2Voxel);

	// Gets the max height from a set of MAP2 voxels.
	int getMap2Height(const BufferView2D<const ArenaTypes::VoxelID> &map2);

	// Gets the voxel height of a .MIF level with optional ceiling data.
	int getMifLevelHeight(const MIFFile::Level &level, const INFFile::CeilingData *ceiling);

	// Gets the offset value of a door voxel in the world. Used with various calculations
	// (.MIF name, lock level).
	uint16_t getDoorVoxelOffset(WEInt x, SNInt y);

	// Gets the .MIF name for a door voxel in a city or the wilderness.
	std::string getDoorVoxelMifName(WEInt x, SNInt y, int menuID, uint32_t rulerSeed,
		bool palaceIsMainQuestDungeon, ArenaTypes::CityType cityType, MapType mapType,
		const ExeData &exeData);

	// Gets the lock level for a door voxel at the given XY coordinate.
	int getDoorVoxelLockLevel(WEInt x, SNInt y, ArenaRandom &random);

	// Gets the '#' number used in IN#.0x and RE#.0x save files.
	int getServiceSaveFileNumber(WEInt doorX, SNInt doorY);
	int getWildernessServiceSaveFileNumber(int wildX, int wildY);
}

#endif
