#include <algorithm>
#include <unordered_map>

#include "ArenaCityUtils.h"
#include "ArenaInteriorUtils.h"
#include "ArenaLevelUtils.h"
#include "ArenaWildUtils.h"
#include "ChunkUtils.h"
#include "MapDefinition.h"
#include "MapGeneration.h"
#include "MapType.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/RMDFile.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Math/Random.h"
#include "../Sky/SkyGeneration.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Weather/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/String.h"

MapDefinitionInterior::MapDefinitionInterior()
{
	this->interiorType = static_cast<ArenaTypes::InteriorType>(-1);
}

void MapDefinitionInterior::init(ArenaTypes::InteriorType interiorType)
{
	this->interiorType = interiorType;
}

void MapDefinitionInterior::clear()
{
	this->interiorType = static_cast<ArenaTypes::InteriorType>(-1);
}

MapDefinitionWild::MapDefinitionWild()
{
	this->fallbackSeed = 0;
}

void MapDefinitionWild::init(Buffer2D<int> &&levelDefIndices, uint32_t fallbackSeed,
	std::vector<MapGeneration::WildChunkBuildingNameInfo> &&buildingNameInfos)
{
	this->levelDefIndices = std::move(levelDefIndices);
	this->fallbackSeed = fallbackSeed;
	this->buildingNameInfos = std::move(buildingNameInfos);
}

int MapDefinitionWild::getLevelDefIndex(const ChunkInt2 &chunk) const
{
	const bool isValid = (chunk.x >= 0) && (chunk.x < this->levelDefIndices.getWidth()) &&
		(chunk.y >= 0) && (chunk.y < this->levelDefIndices.getHeight());

	if (isValid)
	{
		return this->levelDefIndices.get(chunk.x, chunk.y);
	}
	else
	{
		// @todo: use fallbackSeed when outside the defined wild chunks. Not sure yet how
		// to generate a random value between 0 and levelDefCount for an arbitrary Int2
		// without running random.next() some arbitrary number of times.
		return 0;
	}
}

const MapGeneration::WildChunkBuildingNameInfo *MapDefinitionWild::getBuildingNameInfo(const ChunkInt2 &chunk) const
{
	const auto iter = std::find_if(this->buildingNameInfos.begin(), this->buildingNameInfos.end(),
		[&chunk](const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo)
	{
		return buildingNameInfo.getChunk() == chunk;
	});

	return (iter != this->buildingNameInfos.end()) ? &(*iter) : nullptr;
}

void MapDefinitionWild::clear()
{
	this->levelDefIndices.clear();
	this->fallbackSeed = 0;
	this->buildingNameInfos.clear();
}

MapSubDefinition::MapSubDefinition()
{
	this->type = static_cast<MapType>(-1);
}

void MapSubDefinition::clear()
{
	this->type = static_cast<MapType>(-1);
	this->interior.clear();
	this->wild.clear();
}

void MapDefinition::init(MapType mapType)
{
	this->subDefinition.type = mapType;
}

bool MapDefinition::initInteriorLevels(const MIFFile &mif, ArenaTypes::InteriorType interiorType,
	const std::optional<uint32_t> &rulerSeed, const std::optional<bool> &rulerIsMale,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager)
{
	// N level + level info pairs.
	const int levelCount = mif.getLevelCount();
	this->levels.init(levelCount);
	this->levelInfos.init(levelCount);
	this->levelInfoMappings.init(levelCount);
	this->skies.init(levelCount);
	this->skyMappings.init(levelCount);
	this->skyInfos.init(levelCount);
	this->skyInfoMappings.init(levelCount);

	auto initLevelAndInfo = [this, &mif, interiorType, &rulerSeed, &rulerIsMale, &charClassLibrary,
		&entityDefLibrary, &binaryAssetLibrary, &textureManager](int levelIndex, const MIFLevel &mifLevel, const INFFile &inf)
	{
		LevelDefinition &levelDef = this->levels.get(levelIndex);
		LevelInfoDefinition &levelInfoDef = this->levelInfos.get(levelIndex);

		const INFCeiling &ceiling = inf.getCeiling();
		const WEInt levelWidth = mif.getWidth();
		const int levelHeight = ArenaLevelUtils::getMifLevelHeight(mifLevel, &ceiling);
		const SNInt levelDepth = mif.getDepth();

		// Transpose .MIF dimensions to new dimensions.
		levelDef.init(levelDepth, levelHeight, levelWidth);

		const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);
		levelInfoDef.init(ceilingScale);

		// Set LevelDefinition and LevelInfoDefinition voxels and entities from .MIF + .INF together
		// (due to ceiling, etc.).
		const BufferView<const MIFLevel> mifLevelView(&mifLevel, 1);
		constexpr MapType mapType = MapType::Interior;
		constexpr std::optional<bool> palaceIsMainQuestDungeon; // Not necessary for interiors.
		constexpr std::optional<ArenaTypes::CityType> cityType; // Not necessary for interiors.
		constexpr LocationDungeonDefinition *dungeonDef = nullptr; // Not necessary for non-dungeons.
		constexpr std::optional<bool> isArtifactDungeon; // Not necessary for non-dungeons.
		BufferView<LevelDefinition> levelDefView(&levelDef, 1);
		MapGeneration::readMifVoxels(mifLevelView, mapType, interiorType, rulerSeed, rulerIsMale,
			palaceIsMainQuestDungeon, cityType, dungeonDef, isArtifactDungeon, inf, charClassLibrary,
			entityDefLibrary, binaryAssetLibrary, textureManager, levelDefView, &levelInfoDef);
		MapGeneration::readMifLocks(mifLevelView, inf, levelDefView, &levelInfoDef);
		MapGeneration::readMifTriggers(mifLevelView, inf, levelDefView, &levelInfoDef);

		// Generate interior sky.
		SkyGeneration::InteriorSkyGenInfo interiorSkyGenInfo;
		interiorSkyGenInfo.init(ceiling.outdoorDungeon);

		SkyDefinition &skyDef = this->skies.get(levelIndex);
		SkyInfoDefinition &skyInfoDef = this->skyInfos.get(levelIndex);
		SkyGeneration::generateInteriorSky(interiorSkyGenInfo, textureManager, &skyDef, &skyInfoDef);
	};

	for (int i = 0; i < mif.getLevelCount(); i++)
	{
		const MIFLevel &level = mif.getLevel(i);
		const std::string infName = String::toUppercase(level.getInfo());
		INFFile inf;
		if (!inf.init(infName.c_str()))
		{
			DebugLogError("Couldn't init .INF file \"" + infName + "\".");
			return false;
		}

		initLevelAndInfo(i, level, inf);
	}

	// Each interior level info maps to its parallel level.
	for (int i = 0; i < this->levelInfoMappings.getCount(); i++)
	{
		this->levelInfoMappings.set(i, i);
	}

	for (int i = 0; i < this->skyMappings.getCount(); i++)
	{
		this->skyMappings.set(i, i);
	}

	for (int i = 0; i < this->skyInfoMappings.getCount(); i++)
	{
		this->skyInfoMappings.set(i, i);
	}

	this->subDefinition.interior.init(interiorType);

	return true;
}

bool MapDefinition::initDungeonLevels(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks,
	bool isArtifactDungeon, ArenaRandom &random, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, WorldInt2 *outStartPoint)
{
	const int levelCount = ArenaInteriorUtils::generateDungeonLevelCount(isArtifactDungeon, random);

	// N LevelDefinitions all pointing to one LevelInfoDefinition.
	this->levels.init(levelCount);
	this->levelInfos.init(1);
	this->levelInfoMappings.init(levelCount);
	this->skies.init(levelCount);
	this->skyMappings.init(levelCount);
	this->skyInfos.init(levelCount);
	this->skyInfoMappings.init(levelCount);

	// Use the .INF filename of the first level.
	const MIFLevel &level = mif.getLevel(0);
	const std::string infName = String::toUppercase(level.getInfo());
	INFFile inf;
	if (!inf.init(infName.c_str()))
	{
		DebugLogError("Couldn't init .INF file \"" + infName + "\".");
		return false;
	}

	const INFCeiling &ceiling = inf.getCeiling();
	const WEInt levelWidth = mif.getWidth() * widthChunks;
	const int levelHeight = ceiling.outdoorDungeon ? 2 : 3;
	const SNInt levelDepth = mif.getDepth() * depthChunks;

	for (LevelDefinition &levelDef : this->levels)
	{
		// Transpose .MIF dimensions to new dimensions.
		levelDef.init(levelDepth, levelHeight, levelWidth);
	}

	BufferView<LevelDefinition> levelDefView(this->levels);
	LevelInfoDefinition &levelInfoDef = this->levelInfos.get(0);

	const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);
	levelInfoDef.init(ceilingScale);

	constexpr ArenaTypes::InteriorType interiorType = ArenaTypes::InteriorType::Dungeon;
	constexpr std::optional<bool> rulerIsMale;
	MapGeneration::generateMifDungeon(mif, levelCount, widthChunks, depthChunks, inf, random,
		this->subDefinition.type, interiorType, rulerIsMale, isArtifactDungeon, charClassLibrary,
		entityDefLibrary, binaryAssetLibrary, textureManager, levelDefView, &levelInfoDef, outStartPoint);

	// Generate sky for each dungeon level.
	for (int i = 0; i < levelCount; i++)
	{
		SkyGeneration::InteriorSkyGenInfo interiorSkyGenInfo;
		interiorSkyGenInfo.init(ceiling.outdoorDungeon);

		SkyDefinition &skyDef = this->skies.get(i);
		SkyInfoDefinition &skyInfoDef = this->skyInfos.get(i);
		SkyGeneration::generateInteriorSky(interiorSkyGenInfo, textureManager, &skyDef, &skyInfoDef);
	}

	// Each dungeon level uses the same level info definition.
	for (int i = 0; i < this->levelInfoMappings.getCount(); i++)
	{
		this->levelInfoMappings.set(i, 0);
	}

	for (int i = 0; i < this->skyMappings.getCount(); i++)
	{
		this->skyMappings.set(i, i);
	}

	for (int i = 0; i < this->skyInfoMappings.getCount(); i++)
	{
		this->skyInfoMappings.set(i, i);
	}

	this->subDefinition.interior.init(interiorType);

	return true;
}

bool MapDefinition::initCityLevel(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID,
	bool isPremade, BufferView<const uint8_t> reservedBlocks, WEInt blockStartPosX,
	SNInt blockStartPosY, int cityBlocksPerSide, bool coastal, bool rulerIsMale, bool palaceIsMainQuestDungeon,
	const std::string_view cityTypeName, ArenaTypes::CityType cityType,
	const LocationCityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
	const SkyGeneration::ExteriorSkyGenInfo &exteriorSkyGenInfo, const INFFile &inf,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
	TextureManager &textureManager)
{
	// 1 LevelDefinition and 1 LevelInfoDefinition.
	this->levels.init(1);
	this->levelInfos.init(1);
	this->levelInfoMappings.init(1);
	this->skies.init(1);
	this->skyMappings.init(1);
	this->skyInfos.init(1);
	this->skyInfoMappings.init(1);

	const WEInt levelWidth = mif.getWidth();
	const int levelHeight =  6;
	const SNInt levelDepth = mif.getDepth();

	// Transpose .MIF dimensions to new dimensions.
	LevelDefinition &levelDef = this->levels.get(0);
	levelDef.init(levelDepth, levelHeight, levelWidth);

	const INFCeiling &ceiling = inf.getCeiling();
	const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);
	LevelInfoDefinition &levelInfoDef = this->levelInfos.get(0);
	levelInfoDef.init(ceilingScale);

	MapGeneration::generateMifCity(mif, citySeed, rulerSeed, raceID, isPremade, rulerIsMale, palaceIsMainQuestDungeon,
		reservedBlocks, blockStartPosX, blockStartPosY, cityBlocksPerSide, coastal, cityTypeName, cityType,
		mainQuestTempleOverride, inf, charClassLibrary, entityDefLibrary, binaryAssetLibrary, textAssetLibrary,
		textureManager, &levelDef, &levelInfoDef);

	SkyDefinition &skyDef = this->skies.get(0);
	SkyInfoDefinition &skyInfoDef = this->skyInfos.get(0);
	SkyGeneration::generateExteriorSky(exteriorSkyGenInfo, binaryAssetLibrary, textureManager,
		&skyDef, &skyInfoDef);

	// Only one level info and sky to use.
	this->levelInfoMappings.set(0, 0);
	this->skyMappings.set(0, 0);
	this->skyInfoMappings.set(0, 0);
	
	return true;
}

bool MapDefinition::initWildLevels(BufferView2D<const ArenaWildUtils::WildBlockID> wildBlockIDs,
	uint32_t fallbackSeed, const LocationCityDefinition &cityDef,
	const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, const INFFile &inf,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager)
{
	// Create a list of unique block IDs and a 2D table of level definition index mappings. The index
	// of a wild block ID is its level definition index.
	std::vector<ArenaWildUtils::WildBlockID> uniqueWildBlockIDs;
	Buffer2D<int> levelDefIndices(wildBlockIDs.getWidth(), wildBlockIDs.getHeight());
	for (int y = 0; y < wildBlockIDs.getHeight(); y++)
	{
		for (int x = 0; x < wildBlockIDs.getWidth(); x++)
		{
			// Transpose the XY coordinate since the wild block IDs buffer is in original space.
			const ArenaWildUtils::WildBlockID blockID = wildBlockIDs.get(y, x);
			const auto iter = std::find(uniqueWildBlockIDs.begin(), uniqueWildBlockIDs.end(), blockID);

			int levelDefIndex;
			if (iter != uniqueWildBlockIDs.end())
			{
				levelDefIndex = static_cast<int>(std::distance(uniqueWildBlockIDs.begin(), iter));
			}
			else
			{
				uniqueWildBlockIDs.emplace_back(blockID);
				levelDefIndex = static_cast<int>(uniqueWildBlockIDs.size()) - 1;
			}

			levelDefIndices.set(x, y, levelDefIndex);
		}
	}

	// N LevelDefinitions (for chunks) and 1 LevelInfoDefinition.
	this->levels.init(static_cast<int>(uniqueWildBlockIDs.size()));
	this->levelInfos.init(1);
	this->levelInfoMappings.init(this->levels.getCount());
	this->skies.init(1);
	this->skyMappings.init(this->levels.getCount()); // Unnecessary but convenient for API.
	this->skyInfos.init(1);
	this->skyInfoMappings.init(1);

	for (LevelDefinition &levelDef : this->levels)
	{
		// Each .RMD file should be one chunk's width and depth.
		constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
		levelDef.init(chunkDim, 6, chunkDim);
	}

	LevelInfoDefinition &levelInfoDef = this->levelInfos.get(0);
	const INFCeiling &ceiling = inf.getCeiling();
	const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);
	levelInfoDef.init(ceilingScale);

	const BufferView<const ArenaWildUtils::WildBlockID> uniqueWildBlockIdsConstView(uniqueWildBlockIDs);
	const BufferView2D<const int> levelDefIndicesConstView(levelDefIndices);
	BufferView<LevelDefinition> levelDefsView(this->levels);
	std::vector<MapGeneration::WildChunkBuildingNameInfo> buildingNameInfos;
	MapGeneration::generateRmdWilderness(uniqueWildBlockIdsConstView, levelDefIndicesConstView, cityDef, inf,
		charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, levelDefsView, &levelInfoDef,
		&buildingNameInfos);

	SkyDefinition &skyDef = this->skies.get(0);
	SkyInfoDefinition &skyInfoDef = this->skyInfos.get(0);
	SkyGeneration::generateExteriorSky(skyGenInfo, binaryAssetLibrary, textureManager, &skyDef, &skyInfoDef);

	// Every wild chunk level definition uses the same level info definition.
	for (int i = 0; i < this->levelInfoMappings.getCount(); i++)
	{
		this->levelInfoMappings.set(i, 0);
	}

	for (int i = 0; i < this->skyMappings.getCount(); i++)
	{
		this->skyMappings.set(i, 0);
	}

	this->skyInfoMappings.set(0, 0);

	// Populate wild chunk look-up values.
	this->subDefinition.wild.init(std::move(levelDefIndices), fallbackSeed, std::move(buildingNameInfos));

	return true;
}

void MapDefinition::initStartPoints(const MIFFile &mif)
{
	this->startPoints.init(mif.getStartPointCount());
	for (int i = 0; i < mif.getStartPointCount(); i++)
	{
		const OriginalInt2 &mifStartPoint = mif.getStartPoint(i);
		const Double2 mifStartPointReal = MIFUtils::convertStartPointToReal(mifStartPoint);
		this->startPoints.get(i) = VoxelUtils::getTransformedVoxel(mifStartPointReal);
	}
}

bool MapDefinition::initInterior(const MapGeneration::InteriorGenInfo &generationInfo, TextureManager &textureManager)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	this->init(MapType::Interior);

	const MapGeneration::InteriorGenType interiorType = generationInfo.type;
	if (interiorType == MapGeneration::InteriorGenType::Prefab)
	{
		const MapGeneration::InteriorPrefabGenInfo &prefabGenInfo = generationInfo.prefab;
		MIFFile mif;
		if (!mif.init(prefabGenInfo.mifName.c_str()))
		{
			DebugLogError("Couldn't init .MIF file \"" + prefabGenInfo.mifName + "\".");
			return false;
		}

		constexpr std::optional<uint32_t> rulerSeed; // Not necessary for interiors.
		this->initInteriorLevels(mif, prefabGenInfo.interiorType, rulerSeed, prefabGenInfo.rulerIsMale,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager);
		this->initStartPoints(mif);
		this->startLevelIndex = mif.getStartingLevelIndex();
	}
	else if (interiorType == MapGeneration::InteriorGenType::Dungeon)
	{
		const MapGeneration::InteriorDungeonGenInfo &dungeonGenInfo = generationInfo.dungeon;

		// Dungeon .MIF file with chunks for random generation.
		const std::string &mifName = ArenaInteriorUtils::DUNGEON_MIF_NAME;
		MIFFile mif;
		if (!mif.init(mifName.c_str()))
		{
			DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
			return false;
		}

		ArenaRandom random(dungeonGenInfo.dungeonDef.dungeonSeed);

		// Generate dungeon levels and get the player start point.
		WorldInt2 startPoint;
		this->initDungeonLevels(mif, dungeonGenInfo.dungeonDef.widthChunkCount,
			dungeonGenInfo.dungeonDef.heightChunkCount, dungeonGenInfo.isArtifactDungeon, random,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &startPoint);

		const WorldDouble2 startPointReal = VoxelUtils::getVoxelCenter(startPoint);
		this->startPoints.init(1);
		this->startPoints.set(0, startPointReal);
		this->startLevelIndex = 0;
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(interiorType)));
	}
	
	return true;
}

bool MapDefinition::initCity(const MapGeneration::CityGenInfo &generationInfo,
	const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();

	this->init(MapType::City);

	const std::string &mifName = generationInfo.mifName;
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return false;
	}

	const std::string infName = ArenaCityUtils::generateInfName(skyGenInfo.climateType, skyGenInfo.weatherDef.type);
	INFFile inf;
	if (!inf.init(infName.c_str()))
	{
		DebugLogError("Couldn't init .INF file \"" + infName + "\".");
		return false;
	}

	const BufferView<const uint8_t> reservedBlocks(generationInfo.reservedBlocks);
	const LocationCityDefinition::MainQuestTempleOverride *mainQuestTempleOverride =
		generationInfo.mainQuestTempleOverride.has_value() ? &(*generationInfo.mainQuestTempleOverride) : nullptr;

	// Generate city level (optionally generating random city blocks if not premade).
	this->initCityLevel(mif, generationInfo.citySeed, generationInfo.rulerSeed, generationInfo.raceID,
		generationInfo.isPremade, reservedBlocks, generationInfo.blockStartPosX, generationInfo.blockStartPosY,
		generationInfo.cityBlocksPerSide, generationInfo.coastal, generationInfo.rulerIsMale,
		generationInfo.palaceIsMainQuestDungeon, generationInfo.cityTypeName, generationInfo.cityType,
		mainQuestTempleOverride, skyGenInfo, inf, charClassLibrary, entityDefLibrary, binaryAssetLibrary,
		textAssetLibrary, textureManager);
	this->initStartPoints(mif);
	this->startLevelIndex = 0;
	return true;
}

bool MapDefinition::initWild(const MapGeneration::WildGenInfo &generationInfo,
	const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	this->init(MapType::Wilderness);

	const std::string infName = ArenaWildUtils::generateInfName(skyGenInfo.climateType, skyGenInfo.weatherDef.type);
	INFFile inf;
	if (!inf.init(infName.c_str()))
	{
		DebugLogError("Couldn't init .INF file \"" + infName + "\".");
		return false;
	}
	
	const BufferView2D<const ArenaWildUtils::WildBlockID> wildBlockIDs(generationInfo.wildBlockIDs);
	const LocationCityDefinition &cityDef = *generationInfo.cityDef;

	this->initWildLevels(wildBlockIDs, generationInfo.fallbackSeed, cityDef, skyGenInfo, inf,
		charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager);

	// No start level index and no start points in the wilderness due to the nature of chunks.
	this->startLevelIndex = std::nullopt;
	return true;
}

MapType MapDefinition::getMapType() const
{
	return this->subDefinition.type;
}

bool MapDefinition::isValid() const
{
	return static_cast<int>(this->getMapType()) >= 0;
}

const std::optional<int> &MapDefinition::getStartLevelIndex() const
{
	return this->startLevelIndex;
}

int MapDefinition::getStartPointCount() const
{
	return this->startPoints.getCount();
}

const WorldDouble2 &MapDefinition::getStartPoint(int index) const
{
	return this->startPoints.get(index);
}

BufferView<const LevelDefinition> MapDefinition::getLevels() const
{
	return this->levels;
}

BufferView<const int> MapDefinition::getLevelInfoIndices() const
{
	return this->levelInfoMappings;
}

BufferView<const LevelInfoDefinition> MapDefinition::getLevelInfos() const
{
	return this->levelInfos;
}

int MapDefinition::getSkyIndexForLevel(int levelIndex) const
{
	return this->skyMappings.get(levelIndex);
}

const SkyDefinition &MapDefinition::getSky(int index) const
{
	return this->skies.get(index);
}

const SkyInfoDefinition &MapDefinition::getSkyInfoForSky(int skyIndex) const
{
	const int skyInfoIndex = this->skyInfoMappings.get(skyIndex);
	return this->skyInfos.get(skyInfoIndex);
}

const MapSubDefinition &MapDefinition::getSubDefinition() const
{
	return this->subDefinition;
}

void MapDefinition::clear()
{
	this->levels.clear();
	this->levelInfos.clear();
	this->skies.clear();
	this->skyInfos.clear();
	this->levelInfoMappings.clear();
	this->skyMappings.clear();
	this->skyInfoMappings.clear();
	this->startPoints.clear();
	this->startLevelIndex = std::nullopt;
	this->subDefinition.clear();
}
