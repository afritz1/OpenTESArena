#ifndef LOCATION_DEFINITION_H
#define LOCATION_DEFINITION_H

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

struct LocationCityDefinition
{
	// Used with a couple special-cased temple names in the original game.
	struct MainQuestTempleOverride
	{
		int modelIndex;
		int suffixIndex;
		int menuNamesIndex;

		void init(int modelIndex, int suffixIndex, int menuNamesIndex);
	};

	ArenaCityType type;
	char typeDisplayName[16];
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
	MainQuestTempleOverride mainQuestTempleOverride;

	int cityBlocksPerSide;
	bool coastal;
	bool premade; // @todo: should be a nullable data struct instead, telling what kind of premade thing.
	bool rulerIsMale;
	bool palaceIsMainQuestDungeon;

	void init(ArenaCityType type, const char *typeDisplayName, const char *mapFilename,
		uint32_t citySeed, uint32_t wildSeed, uint32_t provinceSeed, uint32_t rulerSeed,
		uint32_t skySeed, ArenaClimateType climateType, const std::vector<uint8_t> *reservedBlocks,
		WEInt blockStartPosX, SNInt blockStartPosY, const MainQuestTempleOverride *mainQuestTempleOverride,
		int cityBlocksPerSide, bool coastal, bool premade, bool rulerIsMale, bool palaceIsMainQuestDungeon);

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

class LocationDefinition
{
private:
	std::string name;
	int x, y;
	double latitude;
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
	void init(LocationDefinitionType type, const std::string &name, int x, int y, double latitude);
public:
	// Initialize from original game data.
	void initCity(int localCityID, int provinceID, bool coastal, bool premade,
		ArenaCityType type, const BinaryAssetLibrary &binaryAssetLibrary);
	void initDungeon(int localDungeonID, int provinceID, const ArenaLocationData &locationData, const ArenaProvinceData &provinceData);
	void initMainQuestDungeon(const std::optional<int> &optLocalDungeonID, int provinceID,
		LocationMainQuestDungeonDefinitionType type, const BinaryAssetLibrary &binaryAssetLibrary);
	// @todo: eventually have init(const char *filename) for custom locations.

	// Gets the display name of the location.
	const std::string &getName() const;

	// Pixel coordinates of the location.
	int getScreenX() const;
	int getScreenY() const;

	// Latitude of the location in normalized [-1, 1] range across the world map's height,
	// where 0 is at the equator and 1.0 is at the north pole.
	double getLatitude() const;

	// Whether the location needs to be discovered for it to be visible on the map.
	bool isVisibleByDefault() const;

	// Gets the location type (city, dungeon, etc.). Determines union access.
	LocationDefinitionType getType() const;

	// Type-specific location fields, accessible by the location type.
	const LocationCityDefinition &getCityDefinition() const;
	const LocationDungeonDefinition &getDungeonDefinition() const;
	const LocationMainQuestDungeonDefinition &getMainQuestDungeonDefinition() const;

	// Returns whether the two definitions reference the same location in a province.
	bool matches(const LocationDefinition &other) const;
};

#endif
