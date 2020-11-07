#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

#include <optional>
#include <string>
#include <vector>

#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "MapGeneration.h"
#include "SkyDefinition.h"
#include "VoxelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView2D.h"

// Modern replacement for .MIF/.RMD files. Helps create a buffer between how the game world data
// is defined and how it's represented in-engine, so that it doesn't care about things like
// chunks.

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class TextureManager;

enum class ClimateType;
enum class WeatherType;
enum class WorldType;

class MapDefinition
{
public:
	class Interior
	{
	private:
		// @todo: interior type (shop, mage's guild, dungeon, etc.)?
		// - InteriorDefinition?
		// - probably store the music filename here, or make it retrievable by the interior type
	public:

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
	Buffer<SkyDefinition> skyDefinitions; // Each can be used by one or more levels.
	Buffer<int> levelInfoMappings; // Level info pointed to by each level.
	Buffer<int> skyDefinitionMappings; // Sky definitions pointed to by each level.
	Buffer<LevelDouble2> startPoints;
	std::optional<int> startLevelIndex;

	// World-type-specific data.
	WorldType worldType;
	Interior interior;
	Wild wild;

	void init(WorldType worldType);
	bool initInteriorLevels(const MIFFile &mif, bool isPalace, const std::optional<bool> &rulerIsMale,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initDungeonLevels(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks,
		bool isArtifactDungeon, ArenaRandom &random, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, LevelInt2 *outStartPoint);
	bool initCityLevel(const MIFFile &mif, uint32_t citySeed, bool isPremade,
		const BufferView<const uint8_t> &reservedBlocks, WEInt blockStartPosX, SNInt blockStartPosY,
		int cityBlocksPerSide, Buffer<Color> &&skyColors, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initWildLevels(const BufferView2D<const WildBlockID> &wildBlockIDs, uint32_t fallbackSeed,
		Buffer<Color> &&skyColors, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);
	void initStartPoints(const MIFFile &mif);
public:
	bool initInterior(const MapGeneration::InteriorGenInfo &generationInfo,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);
	bool initCity(const MapGeneration::CityGenInfo &generationInfo, ClimateType climateType,
		WeatherType weatherType, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);
	bool initWild(const MapGeneration::WildGenInfo &generationInfo, ClimateType climateType,
		WeatherType weatherType, const CharacterClassLibrary &charClassLibrary,
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
	const SkyDefinition &getSkyForLevel(int levelIndex) const;

	WorldType getWorldType() const;
	const Interior &getInterior() const;
	const Wild &getWild() const;
};

#endif
