#include "ArenaCityUtils.h"
#include "ArenaInteriorUtils.h"
#include "ArenaWildUtils.h"
#include "MapType.h"
#include "WorldData.h"
#include "../Assets/MIFUtils.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void WorldData::Interior::init(ArenaTypes::InteriorType interiorType)
{
	this->interiorType = interiorType;
}

WorldData::WorldData(MapType mapType, int activeLevelIndex)
{
	this->mapType = mapType;
	this->activeLevelIndex = activeLevelIndex;
}

WorldData WorldData::loadInterior(ArenaTypes::InteriorType interiorType, const MIFFile &mif, const ExeData &exeData)
{
	WorldData worldData(MapType::Interior, mif.getStartingLevelIndex());
	worldData.interior.init(interiorType);

	// Generate levels.
	for (int i = 0; i < mif.getLevelCount(); i++)
	{
		const MIFFile::Level &level = mif.getLevel(i);
		worldData.levels.emplace_back(LevelData::loadInterior(
			level, mif.getDepth(), mif.getWidth(), exeData));
	}

	// Convert start points from the old coordinate system to the new one.
	for (int i = 0; i < mif.getStartPointCount(); i++)
	{
		const OriginalInt2 &point = mif.getStartPoint(i);
		const Double2 startPointReal = MIFUtils::convertStartPointToReal(point);
		worldData.startPoints.emplace_back(VoxelUtils::getTransformedVoxel(startPointReal));
	}

	return worldData;
}

WorldData WorldData::loadDungeon(uint32_t seed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon,
	const ExeData &exeData)
{
	// Load the .MIF file with all the dungeon chunks in it. Dimensions should be 32x32.
	const std::string mifName = "RANDOM1.MIF";
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugCrash("Could not init .MIF file \"" + mifName + "\".");
	}

	ArenaRandom random(seed);

	// Number of levels in the dungeon.
	const int levelCount = ArenaInteriorUtils::generateDungeonLevelCount(isArtifactDungeon, random);

	// Store the seed for later, to be used with block selection.
	const uint32_t seed2 = random.getSeed();

	// Determine transition blocks (*LEVELUP/*LEVELDOWN) that will appear in the dungeon.
	auto getNextTransBlock = [widthChunks, depthChunks, &random]()
	{
		const SNInt tY = random.next() % depthChunks;
		const WEInt tX = random.next() % widthChunks;
		return ArenaInteriorUtils::packLevelChangeVoxel(tX, tY);
	};

	// Packed coordinates for transition blocks.
	// @todo: maybe this could be an int pair so packing is not required.
	std::vector<int> transitions;

	// Handle initial case where transitions list is empty (for i == 0).
	transitions.push_back(getNextTransBlock());

	// Handle general case for transitions list additions.
	for (int i = 1; i < levelCount; i++)
	{
		int transBlock;
		do
		{
			transBlock = getNextTransBlock();
		} while (transBlock == transitions.back());

		transitions.push_back(transBlock);
	}

	WorldData worldData(MapType::Interior, 0);
	worldData.interior.init(ArenaTypes::InteriorType::Dungeon);

	// .INF filename is the same for each level (RD1.INF).
	const std::string infName = String::toUppercase(mif.getLevel(0).getInfo());

	const SNInt gridWidth = mif.getDepth() * depthChunks;
	const WEInt gridDepth = mif.getWidth() * widthChunks;

	// Generate each level, deciding which dungeon blocks to use.
	for (int i = 0; i < levelCount; i++)
	{
		random.srand(seed2 + i);
		const int levelUpBlock = transitions.at(i);

		// No *LEVELDOWN block on the lowest level.
		const int *levelDownBlock = (i < (levelCount - 1)) ? &transitions.at(i + 1) : nullptr;

		worldData.levels.emplace_back(LevelData::loadDungeon(
			random, mif, levelUpBlock, levelDownBlock, widthChunks, depthChunks, infName,
			gridWidth, gridDepth, exeData));
	}

	// The start point depends on where the level up voxel is on the first level.
	// Convert it from the old coordinate system to the new one.
	WEInt firstTransitionChunkX;
	SNInt firstTransitionChunkZ;
	ArenaInteriorUtils::unpackLevelChangeVoxel(
		transitions.front(), &firstTransitionChunkX, &firstTransitionChunkZ);

	const OriginalDouble2 startPoint(
		0.50 + static_cast<WEDouble>(ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkX)),
		0.50 + static_cast<SNDouble>(ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkZ)));
	worldData.startPoints.emplace_back(VoxelUtils::getTransformedVoxel(startPoint));

	return worldData;
}

WorldData WorldData::loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	const MIFFile &mif, WeatherType weatherType, int currentDay, int starCount,
	const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
	TextureManager &textureManager)
{
	const MIFFile::Level &level = mif.getLevel(0);
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const std::string infName = ArenaCityUtils::generateInfName(cityDef.climateType, weatherType);

	// Generate level data for the city.
	LevelData levelData = LevelData::loadCity(locationDef, provinceDef, level, weatherType, currentDay, starCount,
		infName, mif.getDepth(), mif.getWidth(), binaryAssetLibrary, textAssetLibrary, textureManager);

	// Generate world data from the level data.
	WorldData worldData(MapType::City, 0);
	worldData.levels.emplace_back(std::move(levelData));

	// Convert start points from the old coordinate system to the new one.
	for (int i = 0; i < mif.getStartPointCount(); i++)
	{
		const OriginalInt2 &point = mif.getStartPoint(i);
		const Double2 startPointReal = MIFUtils::convertStartPointToReal(point);
		worldData.startPoints.emplace_back(VoxelUtils::getTransformedVoxel(startPointReal));
	}

	return worldData;
}

WorldData WorldData::loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	WeatherType weatherType, int currentDay, int starCount, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager)
{
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const std::string infName = ArenaWildUtils::generateInfName(cityDef.climateType, weatherType);

	// Load wilderness data (no starting points to load).
	LevelData levelData = LevelData::loadWilderness(locationDef, provinceDef, weatherType, currentDay,
		starCount, infName, binaryAssetLibrary, textureManager);

	// Generate world data from the wilderness data.
	WorldData worldData(MapType::Wilderness, 0);
	worldData.levels.emplace_back(std::move(levelData));
	return worldData;
}

MapType WorldData::getMapType() const
{
	return this->mapType;
}

int WorldData::getActiveLevelIndex() const
{
	return this->activeLevelIndex;
}

int WorldData::getLevelCount() const
{
	return static_cast<int>(this->levels.size());
}

LevelData &WorldData::getActiveLevel()
{
	DebugAssertIndex(this->levels, this->activeLevelIndex);
	return this->levels[this->activeLevelIndex];
}

const LevelData &WorldData::getActiveLevel() const
{
	DebugAssertIndex(this->levels, this->activeLevelIndex);
	return this->levels[this->activeLevelIndex];
}

const std::vector<NewDouble2> &WorldData::getStartPoints() const
{
	return this->startPoints;
}

const WorldData::Interior &WorldData::getInterior() const
{
	DebugAssert(this->mapType == MapType::Interior);
	return this->interior;
}

void WorldData::setActiveLevelIndex(int index)
{
	this->activeLevelIndex = index;
}
