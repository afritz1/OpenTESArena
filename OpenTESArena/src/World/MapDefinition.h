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
class MiscAssets;
class RMDFile;

enum class ClimateType;
enum class WeatherType;
enum class WorldType;

class MapDefinition
{
public:
	struct Interior
	{
		// @todo: interior type (shop, mage's guild, dungeon, etc.)?
	};

	struct City
	{

	};

	class Wild
	{
	private:
		// Each index is a wild chunk pointing into the map's level definitions.
		Buffer2D<int> levelDefIndices;
		uint32_t fallbackSeed; // I.e. the world map location seed.
	public:
		void init(Buffer2D<int> &&levelDefIndices, uint32_t fallbackSeed);

		int getLevelDefIndex(const ChunkInt2 &chunk) const;
	};
private:
	Buffer<LevelDefinition> levels;
	Buffer<LevelInfoDefinition> levelInfos; // Each can be used by one or more levels.
	std::vector<int> levelInfoMappings; // Level info pointed to by each level.
	std::vector<LevelDouble2> startPoints;
	std::optional<int> startLevelIndex;

	// World-type-specific data.
	WorldType worldType;
	Interior interior;
	City city;
	Wild wild;

	void init(WorldType worldType);
public:
	// Initializes a set of interior levels from the given .MIF file and each level's .INF file.
	bool initInterior(const MIFFile &mif);

	// Initializes a set of interior levels from the given dungeon seed and related parameters.
	bool initDungeon(uint32_t dungeonSeed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon);

	// Initializes from the given .MIF level and determines the .INF file to use.
	bool initCity(const MIFFile::Level &level, ClimateType climateType, WeatherType weatherType);

	// Initializes from the given wild block IDs and fallback seed.
	bool initWild(const BufferView2D<const WildBlockID> &wildBlockIDs, uint32_t fallbackSeed,
		ClimateType climateType, WeatherType weatherType, const MiscAssets &miscAssets);

	// Gets the initial level index for the map (if any).
	const std::optional<int> &getStartLevelIndex() const;

	// Starting positions for the player.
	int getStartPointCount() const;
	const LevelDouble2 &getStartPoint(int index) const;

	// Gets the number of levels in the map.
	int getLevelCount() const;

	// This has different semantics based on the world type. For interiors, levels are separated by
	// level up/down transitions. For a city, there is only one level. For the wilderness, it gets
	// the level associated with a wild chunk whose index is acquired by querying some wild chunk
	// coordinate.
	const LevelDefinition &getLevel(int index) const;
	const LevelInfoDefinition &getLevelInfoForLevel(int levelIndex) const;

	WorldType getWorldType() const;
	const Interior &getInterior() const;
	const City &getCity() const;
	const Wild &getWild() const;
};

#endif
