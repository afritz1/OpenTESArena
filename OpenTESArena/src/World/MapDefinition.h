#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <optional>
#include <vector>

#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "VoxelUtils.h"
#include "WildLevelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView2D.h"

// Modern replacement for .MIF/.RMD files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine, so that it doesn't care about things like
// chunks.

// Estimated use cases:
// - Interior: N levels, N level infos
// - City: 1 level, 1 level info
// - Wild: N levels (one per chunk), 1 level info

class INFFile;
class MIFFile;
class RMDFile;

enum class ClimateType;
enum class WeatherType;

class MapDefinition
{
private:
	Buffer<LevelDefinition> levels;
	Buffer<LevelInfoDefinition> levelInfos; // Each can be used by one or more levels.
	std::vector<LevelDouble2> startPoints;
	std::optional<int> startLevelIndex;

	// @todo: mappings of level index to levelInfo index, w/ public getLevelInfoForLevel(int).
public:
	// Initializes a set of interior levels from the given .MIF file and each level's .INF file.
	void initInterior(const MIFFile &mif);

	// Initializes from the given .MIF level and determines the .INF file to use.
	void initCity(const MIFFile::Level &level, ClimateType climateType, WeatherType weatherType);

	// Initializes from all the unique wild block IDs that are required by the wilderness.
	// Each ID is used to load an .RMD file.
	void initWild(const BufferView<const WildBlockID> &wildBlockIDs, ClimateType climateType,
		WeatherType weatherType);

	const std::optional<int> &getStartLevelIndex() const;
	int getStartPointCount() const;
	const LevelDouble2 &getStartPoint(int index) const;
	int getLevelCount() const;
	const LevelDefinition &getLevel(int index) const;
	const LevelInfoDefinition &getLevelInfoForLevel(int levelIndex) const;
};

#endif
