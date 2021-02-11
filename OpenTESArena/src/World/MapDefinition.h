#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "LocationDefinition.h"
#include "MapGeneration.h"
#include "SkyDefinition.h"
#include "SkyGeneration.h"
#include "SkyInfoDefinition.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView2D.h"

// Modern replacement for .MIF/.RMD files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine, so that it doesn't care about things like
// chunks.

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class TextAssetLibrary;
class TextureManager;

enum class ClimateType;
enum class MapType;
enum class WeatherType;

class MapDefinition
{
public:
	class Interior
	{
	private:
		ArenaTypes::InteriorType interiorType;
		// - InteriorDefinition?
		// - probably store the music filename here, or make it retrievable by the interior type
	public:
		void init(ArenaTypes::InteriorType interiorType);

		ArenaTypes::InteriorType getInteriorType() const;
	};

	class Wild
	{
	private:
		// Each index is a wild chunk pointing into the map's level definitions.
		Buffer2D<int> levelDefIndices;
		uint32_t fallbackSeed; // I.e. the world map location seed.

		// Building name infos for each chunk.
		std::vector<MapGeneration::WildChunkBuildingNameInfo> buildingNameInfos;
	public:
		void init(Buffer2D<int> &&levelDefIndices, uint32_t fallbackSeed,
			std::vector<MapGeneration::WildChunkBuildingNameInfo> &&buildingNameInfos);

		int getLevelDefIndex(const ChunkInt2 &chunk) const;
		const MapGeneration::WildChunkBuildingNameInfo *getBuildingNameInfo(const ChunkInt2 &chunk) const;
	};
private:
	Buffer<LevelDefinition> levels;
	Buffer<LevelInfoDefinition> levelInfos; // Each can be used by one or more levels.
	Buffer<SkyDefinition> skies;
	Buffer<SkyInfoDefinition> skyInfos; // Each can be used by one or more skies.
	Buffer<int> levelInfoMappings; // Level info pointed to by each level.
	Buffer<int> skyMappings; // Sky definition pointed to by each level.
	Buffer<int> skyInfoMappings; // Sky info pointed to by each sky.
	Buffer<LevelDouble2> startPoints;
	std::optional<int> startLevelIndex;

	// Map-type-specific data.
	MapType mapType;
	Interior interior;
	Wild wild;

	void init(MapType mapType);
	bool initInteriorLevels(const MIFFile &mif, ArenaTypes::InteriorType interiorType,
		const std::optional<uint32_t> &rulerSeed, const std::optional<bool> &rulerIsMale,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initDungeonLevels(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks,
		bool isArtifactDungeon, ArenaRandom &random, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, LevelInt2 *outStartPoint);
	bool initCityLevel(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID,
		bool isPremade, const BufferView<const uint8_t> &reservedBlocks, WEInt blockStartPosX,
		SNInt blockStartPosY, int cityBlocksPerSide, bool coastal, bool palaceIsMainQuestDungeon,
		const std::string_view &cityTypeName, ArenaTypes::CityType cityType,
		const LocationDefinition::CityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
		const SkyGeneration::ExteriorSkyGenInfo &exteriorSkyGenInfo, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager);
	bool initWildLevels(const BufferView2D<const ArenaWildUtils::WildBlockID> &wildBlockIDs,
		uint32_t fallbackSeed, uint32_t rulerSeed, bool palaceIsMainQuestDungeon,
		ArenaTypes::CityType cityType, const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);
	void initStartPoints(const MIFFile &mif);
public:
	bool initInterior(const MapGeneration::InteriorGenInfo &generationInfo,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initCity(const MapGeneration::CityGenInfo &generationInfo,
		const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager);
	bool initWild(const MapGeneration::WildGenInfo &generationInfo,
		const SkyGeneration::ExteriorSkyGenInfo &exteriorSkyGenInfo, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);

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

	int getSkyIndexForLevel(int levelIndex) const;
	const SkyDefinition &getSky(int index) const;
	const SkyInfoDefinition &getSkyInfoForSky(int skyIndex) const;

	MapType getMapType() const;
	const Interior &getInterior() const;
	const Wild &getWild() const;
};

#endif
