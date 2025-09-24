#ifndef MAP_GENERATION_H
#define MAP_GENERATION_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ArenaWildUtils.h"
#include "LevelDefinition.h"
#include "TransitionType.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Voxels/VoxelDoorDefinition.h"
#include "../Voxels/VoxelUtils.h"
#include "../WorldMap/LocationDefinition.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/Span.h"

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class LevelDefinition;
class LevelInfoDefinition;
class LocationDefinition;
class TextAssetLibrary;
class TextureManager;

enum class MapType;

struct ExeData;

enum class MapGenerationInteriorType
{
	Prefab,
	Dungeon
};

// Input: N .MIF levels + N level-referenced .INFs
// Output: N LevelDefinition / LevelInfoDefinition pairs
struct MapGenerationInteriorPrefabInfo
{
	std::string mifName;
	ArenaInteriorType interiorType;
	std::optional<bool> rulerIsMale;
	std::string displayName;

	void init(const std::string &mifName, ArenaInteriorType interiorType, const std::optional<bool> &rulerIsMale, const std::string &displayName);
};

// Input: RANDOM1.MIF + RD1.INF (loaded internally) + seed + chunk dimensions
// Output: N LevelDefinitions + 1 LevelInfoDefinition
struct MapGenerationInteriorDungeonInfo
{
	LocationDungeonDefinition dungeonDef;
	bool isArtifactDungeon;

	void init(const LocationDungeonDefinition &dungeonDef, bool isArtifactDungeon);
};

// Data for generating an interior map (building interior, wild den, world map dungeon, etc.).
struct MapGenerationInteriorInfo
{
	MapGenerationInteriorType type;
	ArenaInteriorType interiorType;
	MapGenerationInteriorPrefabInfo prefab;
	MapGenerationInteriorDungeonInfo dungeon;

	MapGenerationInteriorInfo();

	void initPrefab(const std::string &mifName, ArenaInteriorType interiorType, const std::optional<bool> &rulerIsMale, const std::string &displayName);
	void initDungeon(const LocationDungeonDefinition &dungeonDef, bool isArtifactDungeon);
};

// Input: 1 .MIF + 1 weather .INF
// Output: 1 LevelDefinition + 1 LevelInfoDefinition
struct MapGenerationCityInfo
{
	std::string mifName;
	std::string cityTypeName;
	ArenaCityType cityType;
	uint32_t citySeed;
	uint32_t rulerSeed;
	int raceID;
	bool isPremade;
	bool coastal;
	bool rulerIsMale;
	bool palaceIsMainQuestDungeon;

	// Affects which types of city blocks are used at generation start.
	Buffer<uint8_t> reservedBlocks;

	std::optional<LocationCityMainQuestTempleOverride> mainQuestTempleOverride;

	// Generation offset from city origin.
	WEInt blockStartPosX;
	SNInt blockStartPosY;

	int cityBlocksPerSide;

	void init(std::string &&mifName, std::string &&cityTypeName, ArenaCityType cityType, uint32_t citySeed,
		uint32_t rulerSeed, int raceID, bool isPremade, bool coastal, bool rulerIsMale,
		bool palaceIsMainQuestDungeon, Buffer<uint8_t> &&reservedBlocks,
		const std::optional<LocationCityMainQuestTempleOverride> &mainQuestTempleOverride,
		WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide);
};

// Input: 70 .RMD files (from asset library) + 1 weather .INF
// Output: 70 LevelDefinitions + 1 LevelInfoDefinition
struct MapGenerationWildInfo
{
	Buffer2D<ArenaWildBlockID> wildBlockIDs;
	const LocationCityDefinition *cityDef;
	uint32_t fallbackSeed;

	void init(Buffer2D<ArenaWildBlockID> &&wildBlockIDs, const LocationCityDefinition &cityDef, uint32_t fallbackSeed);
};

// Building names in the wild are shared per-chunk.
class MapGenerationWildChunkBuildingNameInfo
{
private:
	ChunkInt2 chunk;
	std::unordered_map<ArenaInteriorType, LevelVoxelBuildingNameID> ids;
public:
	void init(const ChunkInt2 &chunk);

	const ChunkInt2 &getChunk() const;
	bool hasBuildingNames() const;
	bool tryGetBuildingNameID(ArenaInteriorType interiorType, LevelVoxelBuildingNameID *outID) const;
	void setBuildingNameID(ArenaInteriorType interiorType, LevelVoxelBuildingNameID id);
};

// Data that can be used when creating an actual transition definition.
struct MapGenerationTransitionInfo
{
	TransitionType transitionType;
	std::optional<ArenaInteriorType> interiorType;
	std::optional<int> menuID; // Arena *MENU ID for transitions.
	std::optional<bool> isLevelUp; // Stairs direction for interior level changes.

	void init(TransitionType transitionType, const std::optional<ArenaInteriorType> &interiorType,
		const std::optional<int> &menuID, const std::optional<bool> &isLevelUp);
};

// Data that can be used when making an actual door definition.
struct MapGenerationDoorInfo
{
	ArenaDoorType doorType;

	// Indices into .INF sounds.
	int openSoundIndex;
	int closeSoundIndex;

	VoxelDoorCloseType closeType;

	void init(ArenaDoorType doorType, int openSoundIndex, int closeSoundIndex, VoxelDoorCloseType closeType);
};

namespace MapGeneration
{
	// Converts .MIF voxels into a more modern voxel + entity format.
	void readMifVoxels(Span<const MIFLevel> levels, MapType mapType,
		const std::optional<ArenaInteriorType> &interiorType, const std::optional<uint32_t> &rulerSeed,
		const std::optional<bool> &rulerIsMale, const std::optional<bool> &palaceIsMainQuestDungeon,
		const std::optional<ArenaCityType> &cityType, const LocationDungeonDefinition *dungeonDef,
		const std::optional<bool> &isArtifactDungeon, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		Span<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);

	// Generates levels from the random chunk .MIF file and converts them to the modern format.
	// Also writes out the player start voxel.
	void generateMifDungeon(const MIFFile &mif, int levelCount, WEInt widthChunks,
		SNInt depthChunks, const INFFile &inf, ArenaRandom &random, MapType mapType,
		ArenaInteriorType interiorType, const std::optional<bool> &rulerIsMale,
		const std::optional<bool> &isArtifactDungeon, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Span<LevelDefinition> &outLevelDefs,
		LevelInfoDefinition *outLevelInfoDef, WorldInt2 *outStartPoint);

	// Generates a level from the city .MIF file, optionally generating random city blocks if it
	// is not a premade city, and converts the level to the modern format.
	void generateMifCity(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID,
		bool isPremade, bool rulerIsMale, bool palaceIsMainQuestDungeon,
		Span<const uint8_t> reservedBlocks, WEInt blockStartPosX, SNInt blockStartPosY,
		int cityBlocksPerSide, bool coastal, const std::string_view cityTypeName, ArenaCityType cityType,
		const LocationCityMainQuestTempleOverride *mainQuestTempleOverride,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef);

	// Generates wilderness chunks from a list of unique wild block IDs. Each block ID maps to the
	// level definition at the same index.
	void generateRmdWilderness(Span<const ArenaWildBlockID> uniqueWildBlockIDs,
		Span2D<const int> levelDefIndices, const LocationCityDefinition &cityDef,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary,const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Span<LevelDefinition> &outLevelDefs,
		LevelInfoDefinition *outLevelInfoDef,
		std::vector<MapGenerationWildChunkBuildingNameInfo> *outBuildingNameInfos);

	void readMifLocks(Span<const MIFLevel> levels, const INFFile &inf,
		Span<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
	void readMifTriggers(Span<const MIFLevel> levels, const INFFile &inf,
		Span<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
}

#endif
