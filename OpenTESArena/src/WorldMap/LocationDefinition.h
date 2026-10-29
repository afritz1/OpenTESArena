#pragma once

#include <optional>
#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Assets/CityDataFile.h"
#include "../Voxels/VoxelUtils.h"

class BinaryAssetLibrary;

struct ExeData;

enum class LocationDefinitionType
{
	City, // City/town/village.
	Dungeon, // Named quest dungeon. Wilderness 'random' den is not a location def.
	MainQuestDungeon // Start, map, or staff dungeon
};

// Used with a couple special-cased temple names in the original game.
struct LocationCityMainQuestTempleOverride
{
	int modelIndex;
	int suffixIndex;
	int menuNamesIndex;

	void init(int modelIndex, int suffixIndex, int menuNamesIndex);
};

struct LocationCityDefinition
{
	ArenaCityType type;
	char typeDisplayName[16];
	char typeDisplayNameLowercase[16];
	char mapFilename[16]; // .MIF name for most/all cases for now.

	uint32_t citySeed;
	uint32_t wildSeed;
	uint32_t provinceSeed; // Used with wilderness dungeons.
	uint32_t rulerSeed;
	uint32_t skySeed;

	ArenaClimateType climateType;

	const std::vector<uint8_t> *reservedBlocks;

	WEInt blockStartPosX; // Start position of city blocks within the city skeleton in original coordinates.
	SNInt blockStartPosY;

	bool hasMainQuestTempleOverride;
	LocationCityMainQuestTempleOverride mainQuestTempleOverride;

	int cityBlocksPerSide;
	bool coastal;
	bool premade; // @todo: should be a nullable data struct instead, telling what kind of premade thing.
	bool rulerIsMale;
	bool palaceIsMainQuestDungeon;

	void init(ArenaCityType type, const char *typeDisplayName, const char *typeDisplayNameLowercase, const char *mapFilename,
		uint32_t citySeed, uint32_t wildSeed, uint32_t provinceSeed, uint32_t rulerSeed, uint32_t skySeed,
		ArenaClimateType climateType, const std::vector<uint8_t> *reservedBlocks, WEInt blockStartPosX, SNInt blockStartPosY,
		const LocationCityMainQuestTempleOverride *mainQuestTempleOverride, int cityBlocksPerSide, bool coastal, bool premade,
		bool rulerIsMale, bool palaceIsMainQuestDungeon);

	uint32_t getWildDungeonSeed(int wildBlockX, int wildBlockY) const;
};

struct LocationDungeonDefinition
{
	uint32_t dungeonSeed;

	int widthChunkCount;
	int heightChunkCount;

	void init(uint32_t dungeonSeed, int widthChunkCount, int heightChunkCount);
};

enum class LocationMainQuestDungeonDefinitionType
{
	Start,
	Map,
	Staff
};

struct LocationMainQuestDungeonDefinition
{
	// @todo: main quest code should look through world map definition to find start dungeon def
	// instead of assuming/hardcoding anything about its .MIF file. At that point, make a
	// StartDungeonDefinition to put the filename/etc. into.

	LocationMainQuestDungeonDefinitionType type;
	char mapFilename[16]; // .MIF name for all cases for now.

	// @todo: misc quest/main quest items?
	// @todo: main quest stage? Main quest splash?

	void init(LocationMainQuestDungeonDefinitionType type, const char *mapFilename);
};

struct LocationDefinition
{
	std::string name;
	int screenX, screenY; // Pixel coordinates.

	// Latitude of the location in normalized [-1, 1] range across the world map's height,
	// where 0 is at the equator and 1.0 is at the north pole.
	double latitude;

	// Whether the location needs to be discovered for it to be visible on the map.
	bool visibleByDefault;

	// Determines union access.
	LocationDefinitionType type;

	union
	{
		LocationCityDefinition city;
		LocationDungeonDefinition dungeon;
		LocationMainQuestDungeonDefinition mainQuest;
	};

	// Internal init method for original game state.
	void init(LocationDefinitionType type, const std::string &name, int screenX, int screenY, double latitude);

	// Initialize from original game data.
	void initCity(int localCityID, int originalProvinceIndex, bool coastal, bool premade, ArenaCityType type, const BinaryAssetLibrary &binaryAssetLibrary);
	void initDungeon(int localDungeonID, int originalProvinceIndex, const ArenaLocationData &locationData, const ArenaProvinceData &provinceData);
	void initMainQuestDungeon(const std::optional<int> &optLocalDungeonID, int originalProvinceIndex,
		LocationMainQuestDungeonDefinitionType type, const BinaryAssetLibrary &binaryAssetLibrary);
	// @todo: eventually have init(const char *filename) for custom locations.

	// Returns whether the two definitions reference the same location in a province.
	bool matches(const LocationDefinition &other) const;
};
