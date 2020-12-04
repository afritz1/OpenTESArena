#ifndef MAP_GENERATION_H
#define MAP_GENERATION_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "LevelDefinition.h"
#include "LocationDefinition.h"
#include "TransitionType.h"
#include "VoxelDefinition.h"
#include "VoxelUtils.h"
#include "WildLevelUtils.h"
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

enum class InteriorType;
enum class WorldType;

namespace MapGeneration
{
	using InteriorGenInfoID = int;

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
			InteriorType interiorType;
			std::optional<bool> rulerIsMale;

			void init(std::string &&mifName, InteriorType interiorType,
				const std::optional<bool> &rulerIsMale);
		};

		// Input: RANDOM1.MIF + RD1.INF (loaded internally) + seed + chunk dimensions
		// Output: N LevelDefinitions + 1 LevelInfoDefinition
		struct Dungeon
		{
			uint32_t dungeonSeed;
			WEInt widthChunks;
			SNInt depthChunks;
			bool isArtifactDungeon;

			void init(uint32_t dungeonSeed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon);
		};
	private:
		Type type;
		Prefab prefab;
		Dungeon dungeon;

		void init(Type type);
	public:
		InteriorGenInfo();

		void initPrefab(std::string &&mifName, InteriorType interiorType,
			const std::optional<bool> &rulerIsMale);
		void initDungeon(uint32_t dungeonSeed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon);

		Type getType() const;
		const Prefab &getPrefab() const;
		const Dungeon &getDungeon() const;
	};

	// Input: 1 .MIF + 1 weather .INF
	// Output: 1 LevelDefinition + 1 LevelInfoDefinition
	struct CityGenInfo
	{
		std::string mifName;
		std::string cityTypeName;
		uint32_t citySeed;
		int raceID;
		bool isPremade;
		bool coastal;

		// Affects which types of city blocks are used at generation start.
		Buffer<uint8_t> reservedBlocks;

		std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride;

		// Generation offset from city origin.
		WEInt blockStartPosX;
		SNInt blockStartPosY;

		int cityBlocksPerSide;

		void init(std::string &&mifName, std::string &&cityTypeName, uint32_t citySeed, int raceID,
			bool isPremade, bool coastal, Buffer<uint8_t> &&reservedBlocks,
			const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> *mainQuestTempleOverride,
			WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide);
	};

	// Input: 70 .RMD files (from asset library) + 1 weather .INF
	// Output: 70 LevelDefinitions + 1 LevelInfoDefinition
	struct WildGenInfo
	{
		Buffer2D<WildBlockID> wildBlockIDs;
		uint32_t fallbackSeed;

		void init(Buffer2D<WildBlockID> &&wildBlockIDs, uint32_t fallbackSeed);
	};

	// Building names in the wild are shared per-chunk.
	class WildChunkBuildingNameInfo
	{
	private:
		ChunkInt2 chunk;
		// @todo: could this use InteriorType instead of MenuType now?
		std::unordered_map<VoxelDefinition::WallData::MenuType, LevelDefinition::BuildingNameID> ids;
	public:
		void init(const ChunkInt2 &chunk);

		const ChunkInt2 &getChunk() const;
		bool hasBuildingNames() const;
		bool tryGetBuildingNameID(VoxelDefinition::WallData::MenuType menuType, LevelDefinition::BuildingNameID *outID) const;
		void setBuildingNameID(VoxelDefinition::WallData::MenuType menuType, LevelDefinition::BuildingNameID id);
	};

	// Data that can be used when creating an actual transition definition.
	struct TransitionGenInfo
	{
		TransitionType transitionType;
		std::optional<InteriorType> interiorType;
		std::optional<bool> isLevelUp; // Stairs direction for interior level changes.

		void init(TransitionType transitionType, const std::optional<InteriorType> &interiorType,
			const std::optional<bool> &isLevelUp);
	};

	// Converts .MIF voxels into a more modern voxel + entity format.
	void readMifVoxels(const BufferView<const MIFFile::Level> &levels, WorldType worldType,
		const std::optional<InteriorType> &interiorType, const std::optional<bool> &rulerIsMale,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs,
		LevelInfoDefinition *outLevelInfoDef);

	// Generates levels from the random chunk .MIF file and converts them to the modern format.
	// Also writes out the player start voxel.
	void generateMifDungeon(const MIFFile &mif, int levelCount, WEInt widthChunks,
		SNInt depthChunks, const INFFile &inf, ArenaRandom &random, WorldType worldType,
		InteriorType interiorType, const std::optional<bool> &rulerIsMale,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef,
		LevelInt2 *outStartPoint);

	// Generates a level from the city .MIF file, optionally generating random city blocks if it
	// is not a premade city, and converts the level to the modern format.
	void generateMifCity(const MIFFile &mif, uint32_t citySeed, int raceID, bool isPremade,
		const BufferView<const uint8_t> &reservedBlocks, WEInt blockStartPosX, SNInt blockStartPosY,
		int cityBlocksPerSide, bool coastal, const std::string_view &cityTypeName,
		const LocationDefinition::CityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef);

	// Generates wilderness chunks from a list of unique wild block IDs. Each block ID maps to the
	// level definition at the same index.
	void generateRmdWilderness(const BufferView<const WildBlockID> &uniqueWildBlockIDs,
		const BufferView2D<const int> &levelDefIndices, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef,
		std::vector<MapGeneration::WildChunkBuildingNameInfo> *outBuildingNameInfos);

	void readMifLocks(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
	void readMifTriggers(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
}

#endif
