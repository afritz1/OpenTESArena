#include <unordered_map>

#include "CityWorldUtils.h"
#include "InteriorLevelUtils.h"
#include "InteriorWorldUtils.h"
#include "MapDefinition.h"
#include "WorldType.h"
#include "WildWorldUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void MapDefinition::Wild::init(Buffer2D<int> &&levelDefIndices, uint32_t fallbackSeed)
{
	this->levelDefIndices = std::move(levelDefIndices);
	this->fallbackSeed = fallbackSeed;
}

int MapDefinition::Wild::getLevelDefIndex(const ChunkInt2 &chunk) const
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

void MapDefinition::init(WorldType worldType)
{
	this->worldType = worldType;
}

bool MapDefinition::initInterior(const MIFFile &mif)
{
	this->init(WorldType::Interior);

	auto initLevelDef = [&mif](LevelDefinition &levelDef, const MIFFile::Level &level,
		const INFFile &inf)
	{
		const WEInt mifWidth = mif.getWidth();
		const SNInt mifDepth = mif.getDepth();
		levelDef.initInterior(level, mifWidth, mifDepth, &inf.getCeiling());
	};

	this->levels.init(mif.getLevelCount());
	this->levelInfos.init(mif.getLevelCount());
	for (int i = 0; i < mif.getLevelCount(); i++)
	{
		const MIFFile::Level &level = mif.getLevel(i);
		const std::string infName = String::toUppercase(level.getInfo());
		INFFile inf;
		if (!inf.init(infName.c_str()))
		{
			DebugLogError("Couldn't init .INF file \"" + infName +
				"\" for interior level " + std::to_string(i) + ".");
			return false;
		}

		LevelDefinition &levelDef = this->levels.get(i);
		initLevelDef(levelDef, level, inf);

		LevelInfoDefinition &levelInfoDef = this->levelInfos.get(i);
		levelInfoDef.init(inf);

		// Pair level with its level info.
		this->levelInfoMappings.push_back(i);
	}

	for (int i = 0; i < mif.getStartPointCount(); i++)
	{
		const OriginalInt2 &mifStartPoint = mif.getStartPoint(i);		
		const Double2 mifStartPointReal = MIFUtils::convertStartPointToReal(mifStartPoint);
		this->startPoints.push_back(VoxelUtils::getTransformedVoxel(mifStartPointReal));
	}

	this->startLevelIndex = mif.getStartingLevelIndex();
	return true;
}

bool MapDefinition::initDungeon(uint32_t dungeonSeed, WEInt widthChunks, SNInt depthChunks, bool isArtifactDungeon)
{
	this->init(WorldType::Interior);

	// @todo: .INF filename is the same for each level (RD1.INF), but don't have to make that assumption here.

	/*// Load the .MIF file with all the dungeon chunks in it. Dimensions should be 32x32.
	const std::string mifName = "RANDOM1.MIF";
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return false;
	}

	ArenaRandom random(dungeonSeed);

	// Number of levels in the dungeon.
	const int levelCount = InteriorWorldUtils::generateDungeonLevelCount(isArtifactDungeon, random);

	// Store the seed for later, to be used with block selection.
	const uint32_t seed2 = random.getSeed();

	// Determine transition blocks (*LEVELUP/*LEVELDOWN) that will appear in the dungeon.
	auto getNextTransBlock = [widthChunks, depthChunks, &random]()
	{
		const SNInt tY = random.next() % depthChunks;
		const WEInt tX = random.next() % widthChunks;
		return InteriorLevelUtils::packLevelChangeVoxel(tX, tY);
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

	// .INF filename is the same for each level (RD1.INF).
	const std::string infName = String::toUppercase(mif.getLevel(0).getInfo());

	InteriorWorldData worldData;
	const SNInt gridWidth = mif.getDepth() * depthChunks;
	const WEInt gridDepth = mif.getWidth() * widthChunks;

	// Generate each level, deciding which dungeon blocks to use.
	for (int i = 0; i < levelCount; i++)
	{
		random.srand(seed2 + i);
		const int levelUpBlock = transitions.at(i);

		// No *LEVELDOWN block on the lowest level.
		const int *levelDownBlock = (i < (levelCount - 1)) ? &transitions.at(i + 1) : nullptr;

		worldData.levels.push_back(InteriorLevelData::loadDungeon(
			random, mif, levelUpBlock, levelDownBlock, widthChunks, depthChunks, infName,
			gridWidth, gridDepth, exeData));
	}

	// The start point depends on where the level up voxel is on the first level.
	// Convert it from the old coordinate system to the new one.
	WEInt firstTransitionChunkX;
	SNInt firstTransitionChunkZ;
	InteriorLevelUtils::unpackLevelChangeVoxel(
		transitions.front(), &firstTransitionChunkX, &firstTransitionChunkZ);

	const OriginalDouble2 startPoint(
		0.50 + static_cast<WEDouble>(InteriorLevelUtils::offsetLevelChangeVoxel(firstTransitionChunkX)),
		0.50 + static_cast<SNDouble>(InteriorLevelUtils::offsetLevelChangeVoxel(firstTransitionChunkZ)));
	worldData.startPoints.push_back(VoxelUtils::getTransformedVoxel(startPoint));

	worldData.levelIndex = 0;*/

	DebugNotImplemented();
	return true;
}

bool MapDefinition::initCity(const MIFFile::Level &level, ClimateType climateType,
	WeatherType weatherType)
{
	this->init(WorldType::City);
	const DOSUtils::FilenameBuffer infName = CityWorldUtils::generateInfName(climateType, weatherType);
	DebugNotImplemented();
	return true;
}

bool MapDefinition::initWild(const BufferView2D<const WildBlockID> &wildBlockIDs, uint32_t fallbackSeed,
	ClimateType climateType, WeatherType weatherType, const BinaryAssetLibrary &binaryAssetLibrary)
{
	this->init(WorldType::Wilderness);

	auto initLevelDef = [this](LevelDefinition &levelDef, const RMDFile &rmd)
	{
		levelDef.initWild(rmd);
	};

	// Generate a level definition index for each unique wild block ID.
	std::unordered_map<WildBlockID, int> blockLevelDefMappings;
	for (int y = 0; y < wildBlockIDs.getHeight(); y++)
	{
		for (int x = 0; x < wildBlockIDs.getWidth(); x++)
		{
			const WildBlockID blockID = wildBlockIDs.get(x, y);
			const auto iter = blockLevelDefMappings.find(blockID);
			if (iter == blockLevelDefMappings.end())
			{
				const int levelDefIndex = static_cast<int>(blockLevelDefMappings.size());
				blockLevelDefMappings.emplace(blockID, levelDefIndex);
			}
		}
	}

	const int levelDefCount = static_cast<int>(blockLevelDefMappings.size());
	this->levels.init(levelDefCount);
	for (const auto &pair : blockLevelDefMappings)
	{
		const WildBlockID blockID = pair.first;
		const int levelDefIndex = pair.second;

		const RMDFile &rmd = [&binaryAssetLibrary, &wildBlockIDs, blockID]() -> const RMDFile&
		{
			const auto &rmdFiles = binaryAssetLibrary.getWildernessChunks();
			const int rmdIndex = DebugMakeIndex(rmdFiles, blockID - 1);
			return rmdFiles[rmdIndex];
		}();

		LevelDefinition &levelDef = this->levels.get(levelDefIndex);
		initLevelDef(levelDef, rmd);
	}

	const DOSUtils::FilenameBuffer infName = WildWorldUtils::generateInfName(climateType, weatherType);
	INFFile inf;
	if (!inf.init(infName.data()))
	{
		DebugLogError("Couldn't init .INF file \"" + std::string(infName.data()) + "\".");
		return false;
	}

	// Add level info pointed to by all wild chunks.
	LevelInfoDefinition levelInfoDef;
	levelInfoDef.init(inf);
	this->levelInfos.init(1);
	this->levelInfos.set(0, std::move(levelInfoDef));

	for (int i = 0; i < this->levels.getCount(); i++)
	{
		const int levelInfoIndex = 0;
		this->levelInfoMappings.push_back(levelInfoIndex);
	}

	return true;
}

const std::optional<int> &MapDefinition::getStartLevelIndex() const
{
	return this->startLevelIndex;
}

int MapDefinition::getStartPointCount() const
{
	return static_cast<int>(this->startPoints.size());
}

const LevelDouble2 &MapDefinition::getStartPoint(int index) const
{
	DebugAssertIndex(this->startPoints, index);
	return this->startPoints[index];
}

int MapDefinition::getLevelCount() const
{
	return this->levels.getCount();
}

const LevelDefinition &MapDefinition::getLevel(int index) const
{
	return this->levels.get(index);
}

const LevelInfoDefinition &MapDefinition::getLevelInfoForLevel(int levelIndex) const
{
	DebugAssertIndex(this->levelInfoMappings, levelIndex);
	const int levelInfoIndex = this->levelInfoMappings[levelIndex];
	return this->levelInfos.get(levelInfoIndex);
}

WorldType MapDefinition::getWorldType() const
{
	return this->worldType;
}

const MapDefinition::Interior &MapDefinition::getInterior() const
{
	DebugAssert(this->worldType == WorldType::Interior);
	return this->interior;
}

const MapDefinition::City &MapDefinition::getCity() const
{
	DebugAssert(this->worldType == WorldType::City);
	return this->city;
}

const MapDefinition::Wild &MapDefinition::getWild() const
{
	DebugAssert(this->worldType == WorldType::Wilderness);
	return this->wild;
}
