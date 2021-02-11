#ifndef MAP_GENERATION_H
#define MAP_GENERATION_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ArenaWildUtils.h"
#include "LevelDefinition.h"
#include "LocationDefinition.h"
#include "TransitionType.h"
#include "VoxelDefinition.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class ExeData;
class LevelDefinition;
class LevelInfoDefinition;
class LocationDefinition;
class TextAssetLibrary;
class TextureManager;

enum class MapType;

namespace MapGeneration
{
	// Data for generating an interior map (building interior, wild den, world map dungeon, etc.).
	class InteriorGenInfo
	{
	public:
		enum class Type { Prefab, Dungeon };

		// Input: N .MIF levels + N level-referenced .INFs
		// Output: N LevelDefinition / LevelInfoDefinition pairs
		struct Prefab
		{
			std::string mifName;
			ArenaTypes::InteriorType interiorType;
			std::optional<bool> rulerIsMale;

			void init(std::string &&mifName, ArenaTypes::InteriorType interiorType,
				const std::optional<bool> &rulerIsMale);
		};

		// Input: RANDOM1.MIF + RD1.INF (loaded internally) + seed + chunk dimensions
		// Output: N LevelDefinitions + 1 LevelInfoDefinition
		struct Dungeon
		{
			const LocationDefinition::DungeonDefinition *dungeonDef;
			bool isArtifactDungeon;

			void init(const LocationDefinition::DungeonDefinition *dungeonDef, bool isArtifactDungeon);
		};
	private:
		Type type;
		Prefab prefab;
		Dungeon dungeon;

		void init(Type type);
	public:
		InteriorGenInfo();

		void initPrefab(std::string &&mifName, ArenaTypes::InteriorType interiorType,
			const std::optional<bool> &rulerIsMale);
		void initDungeon(const LocationDefinition::DungeonDefinition *dungeonDef, bool isArtifactDungeon);

		Type getType() const;
		const Prefab &getPrefab() const;
		const Dungeon &getDungeon() const;
		ArenaTypes::InteriorType getInteriorType() const;
	};

	// Input: 1 .MIF + 1 weather .INF
	// Output: 1 LevelDefinition + 1 LevelInfoDefinition
	struct CityGenInfo
	{
		std::string mifName;
		std::string cityTypeName;
		ArenaTypes::CityType cityType;
		uint32_t citySeed;
		uint32_t rulerSeed;
		int raceID;
		bool isPremade;
		bool coastal;
		bool palaceIsMainQuestDungeon;

		// Affects which types of city blocks are used at generation start.
		Buffer<uint8_t> reservedBlocks;

		std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride;

		// Generation offset from city origin.
		WEInt blockStartPosX;
		SNInt blockStartPosY;

		int cityBlocksPerSide;

		void init(std::string &&mifName, std::string &&cityTypeName, ArenaTypes::CityType cityType, uint32_t citySeed,
			uint32_t rulerSeed, int raceID, bool isPremade, bool coastal, bool palaceIsMainQuestDungeon,
			Buffer<uint8_t> &&reservedBlocks,
			const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> *mainQuestTempleOverride,
			WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide);
	};

	// Input: 70 .RMD files (from asset library) + 1 weather .INF
	// Output: 70 LevelDefinitions + 1 LevelInfoDefinition
	struct WildGenInfo
	{
		Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs;
		ArenaTypes::CityType cityType;
		uint32_t fallbackSeed;
		uint32_t rulerSeed;
		bool palaceIsMainQuestDungeon;

		void init(Buffer2D<ArenaWildUtils::WildBlockID> &&wildBlockIDs, ArenaTypes::CityType cityType,
			uint32_t fallbackSeed, uint32_t rulerSeed, bool palaceIsMainQuestDungeon);
	};

	// Building names in the wild are shared per-chunk.
	class WildChunkBuildingNameInfo
	{
	private:
		ChunkInt2 chunk;
		std::unordered_map<ArenaTypes::InteriorType, LevelDefinition::BuildingNameID> ids;
	public:
		void init(const ChunkInt2 &chunk);

		const ChunkInt2 &getChunk() const;
		bool hasBuildingNames() const;
		bool tryGetBuildingNameID(ArenaTypes::InteriorType interiorType, LevelDefinition::BuildingNameID *outID) const;
		void setBuildingNameID(ArenaTypes::InteriorType interiorType, LevelDefinition::BuildingNameID id);
	};

	// Data that can be used when creating an actual transition definition.
	struct TransitionDefGenInfo
	{
		TransitionType transitionType;
		std::optional<ArenaTypes::InteriorType> interiorType;
		std::optional<int> menuID; // Arena *MENU ID for transitions.
		std::optional<bool> isLevelUp; // Stairs direction for interior level changes.

		void init(TransitionType transitionType, const std::optional<ArenaTypes::InteriorType> &interiorType,
			const std::optional<int> &menuID, const std::optional<bool> &isLevelUp);
	};

	// Converts .MIF voxels into a more modern voxel + entity format.
	void readMifVoxels(const BufferView<const MIFFile::Level> &levels, MapType mapType,
		const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<uint32_t> &rulerSeed,
		const std::optional<bool> &rulerIsMale, const std::optional<bool> &palaceIsMainQuestDungeon,
		const std::optional<ArenaTypes::CityType> &cityType, const LocationDefinition::DungeonDefinition *dungeonDef,
		const std::optional<bool> &isArtifactDungeon, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);

	// Generates levels from the random chunk .MIF file and converts them to the modern format.
	// Also writes out the player start voxel.
	void generateMifDungeon(const MIFFile &mif, int levelCount, WEInt widthChunks,
		SNInt depthChunks, const INFFile &inf, ArenaRandom &random, MapType mapType,
		ArenaTypes::InteriorType interiorType, const std::optional<bool> &rulerIsMale,
		const std::optional<bool> &isArtifactDungeon, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs,
		LevelInfoDefinition *outLevelInfoDef, LevelInt2 *outStartPoint);

	// Generates a level from the city .MIF file, optionally generating random city blocks if it
	// is not a premade city, and converts the level to the modern format.
	void generateMifCity(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID,
		bool isPremade, bool palaceIsMainQuestDungeon, const BufferView<const uint8_t> &reservedBlocks,
		WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide, bool coastal,
		const std::string_view &cityTypeName, ArenaTypes::CityType cityType,
		const LocationDefinition::CityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef);

	// Generates wilderness chunks from a list of unique wild block IDs. Each block ID maps to the
	// level definition at the same index.
	void generateRmdWilderness(const BufferView<const ArenaWildUtils::WildBlockID> &uniqueWildBlockIDs,
		const BufferView2D<const int> &levelDefIndices, uint32_t rulerSeed, bool palaceIsMainQuestDungeon,
		ArenaTypes::CityType cityType, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary,const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs,
		LevelInfoDefinition *outLevelInfoDef,
		std::vector<MapGeneration::WildChunkBuildingNameInfo> *outBuildingNameInfos);

	void readMifLocks(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
	void readMifTriggers(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
}

#endif
