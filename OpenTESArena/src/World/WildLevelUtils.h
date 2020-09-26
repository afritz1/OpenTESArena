#ifndef WILD_LEVEL_UTILS_H
#define WILD_LEVEL_UTILS_H

#include "LevelUtils.h"
#include "VoxelUtils.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"

#include "components/utilities/Buffer2D.h"

class LocationDefinition;
class VoxelGrid;

using WildBlockID = uint8_t; // Corresponds to WILD{...}.MIF file.

namespace WildLevelUtils
{
	// Chunk counts across wilderness width and height.
	constexpr int WILD_WIDTH = 64;
	constexpr int WILD_HEIGHT = WILD_WIDTH;

	// Wilderness indices for looking up WILD{...}.MIF files, generated once per world map location.
	Buffer2D<WildBlockID> generateWildernessIndices(uint32_t wildSeed,
		const ExeData::Wilderness &wildData);

	// Creates mappings of wilderness *MENU voxel coordinates to *MENU names.
	LevelUtils::MenuNamesList generateWildChunkBuildingNames(const VoxelGrid &voxelGrid,
		const ExeData &exeData);

	// Changes the default filler city skeleton to the one intended for the city.
	void reviseWildernessCity(const LocationDefinition &locationDef, Buffer2D<uint16_t> &flor,
		Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2, const MiscAssets &miscAssets);

	// Gets the origin of a virtual 128x128 space in the wild as if the player was at the given
	// position. This space always contains 4 wild chunks.
	// @todo: when changing to chunks, probably use chunk X and Y here instead of absolute [0,4095],
	// and return the chunk coordinate that contains the origin.
	OriginalInt2 getRelativeWildOrigin(const Int2 &voxel);

	// A variation on getRelativeWildOrigin() -- determine which one is actually what we want for
	// all cases, because getRelativeWildOrigin() apparently doesn't make the automap centered.
	// Given coordinates are expected to be in original coordinate system.
	NewInt2 getCenteredWildOrigin(const NewInt2 &voxel);
}

#endif
