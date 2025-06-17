#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "MapGeneration.h"
#include "../Assets/ArenaTypes.h"
#include "../Sky/SkyDefinition.h"
#include "../Sky/SkyGeneration.h"
#include "../Sky/SkyInfoDefinition.h"
#include "../Voxels/VoxelUtils.h"
#include "../WorldMap/LocationDefinition.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span2D.h"

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class TextAssetLibrary;
class TextureManager;

enum class MapType;

struct MapDefinitionInterior
{
	ArenaInteriorType interiorType;
	// - InteriorDefinition?
	// - probably store the music filename here, or make it retrievable by the interior type

	MapDefinitionInterior();

	void init(ArenaInteriorType interiorType);

	void clear();
};

struct MapDefinitionWild
{
	// Each index is a wild chunk pointing into the map's level definitions.
	Buffer2D<int> levelDefIndices;
	uint32_t fallbackSeed; // I.e. the world map location seed.

	// Building name infos for each chunk.
	std::vector<MapGenerationWildChunkBuildingNameInfo> buildingNameInfos;

	MapDefinitionWild();

	void init(Buffer2D<int> &&levelDefIndices, uint32_t fallbackSeed,
		std::vector<MapGenerationWildChunkBuildingNameInfo> &&buildingNameInfos);

	int getLevelDefIndex(const ChunkInt2 &chunk) const;
	const MapGenerationWildChunkBuildingNameInfo *getBuildingNameInfo(const ChunkInt2 &chunk) const;

	void clear();
};

// Map-type-specific data.
struct MapSubDefinition
{
	MapType type;
	MapDefinitionInterior interior;
	MapDefinitionWild wild;

	MapSubDefinition();

	void clear();
};

// Modern replacement for .MIF/.RMD files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine so it doesn't have to care about chunks.
class MapDefinition
{
private:
	Buffer<LevelDefinition> levels;
	Buffer<LevelInfoDefinition> levelInfos; // Each can be used by one or more levels.
	Buffer<SkyDefinition> skies;
	Buffer<SkyInfoDefinition> skyInfos; // Each can be used by one or more skies.
	Buffer<int> levelInfoMappings; // Level info pointed to by each level.
	Buffer<int> skyMappings; // Sky definition pointed to by each level.
	Buffer<int> skyInfoMappings; // Sky info pointed to by each sky.
	Buffer<WorldDouble2> startPoints;
	std::optional<int> startLevelIndex;
	MapSubDefinition subDefinition;

	void init(MapType mapType);
	bool initInteriorLevels(const MIFFile &mif, ArenaInteriorType interiorType, const std::optional<uint32_t> &rulerSeed,
		const std::optional<bool> &rulerIsMale, const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initDungeonLevels(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon, ArenaRandom &random,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, WorldInt2 *outStartPoint);
	bool initCityLevel(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID, bool isPremade, Span<const uint8_t> reservedBlocks,
		WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide, bool coastal, bool rulerIsMale, bool palaceIsMainQuestDungeon,
		const std::string_view cityTypeName, ArenaCityType cityType, const LocationCityMainQuestTempleOverride *mainQuestTempleOverride,
		const SkyGenerationExteriorInfo &exteriorSkyGenInfo, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager);
	bool initWildLevels(Span2D<const ArenaWildBlockID> wildBlockIDs, uint32_t fallbackSeed, const LocationCityDefinition &cityDef,
		const SkyGenerationExteriorInfo &skyGenInfo, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	void initStartPoints(const MIFFile &mif);
public:
	bool initInterior(const MapGenerationInteriorInfo &generationInfo, TextureManager &textureManager);
	bool initCity(const MapGenerationCityInfo &generationInfo, const SkyGenerationExteriorInfo &skyGenInfo, TextureManager &textureManager);
	bool initWild(const MapGenerationWildInfo &generationInfo, const SkyGenerationExteriorInfo &skyGenInfo, TextureManager &textureManager);

	MapType getMapType() const;
	bool isValid() const;

	// Gets the initial level index for the map (if any).
	const std::optional<int> &getStartLevelIndex() const;

	// Starting positions for the player.
	int getStartPointCount() const;
	const WorldDouble2 &getStartPoint(int index) const;

	// This has different semantics based on the world type. For interiors, levels are separated by
	// level up/down transitions. For a city, there is only one level. For the wilderness, it gets
	// the level associated with a wild chunk whose index is acquired by querying some wild chunk
	// coordinate.
	Span<const LevelDefinition> getLevels() const;

	// Gets the indices for accessing the level info of a level. It's possible that several levels
	// use the same level info.
	Span<const int> getLevelInfoIndices() const;
	Span<const LevelInfoDefinition> getLevelInfos() const;

	int getSkyIndexForLevel(int levelIndex) const;
	const SkyDefinition &getSky(int index) const;
	const SkyInfoDefinition &getSkyInfoForSky(int skyIndex) const;

	const MapSubDefinition &getSubDefinition() const;

	void clear();
};

#endif
