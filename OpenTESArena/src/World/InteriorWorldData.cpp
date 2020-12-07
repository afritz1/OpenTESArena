#include "ArenaInteriorUtils.h"
#include "InteriorWorldData.h"
#include "WorldType.h"
#include "../Assets/MIFUtils.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

InteriorWorldData::InteriorWorldData()
{
	this->interiorType = ArenaTypes::MenuType::None;
	this->levelIndex = 0;
}

InteriorWorldData::~InteriorWorldData()
{

}

InteriorWorldData InteriorWorldData::loadInterior(ArenaTypes::MenuType interiorType,
	const MIFFile &mif, const ExeData &exeData)
{
	InteriorWorldData worldData;

	// Generate levels.
	for (int i = 0; i < mif.getLevelCount(); i++)
	{
		const MIFFile::Level &level = mif.getLevel(i);
		worldData.levels.push_back(InteriorLevelData::loadInterior(
			level, mif.getDepth(), mif.getWidth(), exeData));
	}

	// Convert start points from the old coordinate system to the new one.
	for (int i = 0; i < mif.getStartPointCount(); i++)
	{
		const OriginalInt2 &point = mif.getStartPoint(i);
		const Double2 startPointReal = MIFUtils::convertStartPointToReal(point);
		worldData.startPoints.push_back(VoxelUtils::getTransformedVoxel(startPointReal));
	}

	worldData.levelIndex = mif.getStartingLevelIndex();
	worldData.interiorType = interiorType;

	return worldData;
}

InteriorWorldData InteriorWorldData::loadDungeon(uint32_t seed, WEInt widthChunks, SNInt depthChunks,
	bool isArtifactDungeon, ArenaTypes::MenuType interiorType,
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
	ArenaInteriorUtils::unpackLevelChangeVoxel(
		transitions.front(), &firstTransitionChunkX, &firstTransitionChunkZ);

	const OriginalDouble2 startPoint(
		0.50 + static_cast<WEDouble>(ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkX)),
		0.50 + static_cast<SNDouble>(ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkZ)));
	worldData.startPoints.push_back(VoxelUtils::getTransformedVoxel(startPoint));

	worldData.levelIndex = 0;
	worldData.interiorType = interiorType;

	return worldData;
}

int InteriorWorldData::getLevelIndex() const
{
	return this->levelIndex;
}

int InteriorWorldData::getLevelCount() const
{
	return static_cast<int>(this->levels.size());
}

ArenaTypes::MenuType InteriorWorldData::getInteriorType() const
{
	return this->interiorType;
}

WorldType InteriorWorldData::getWorldType() const
{
	return WorldType::Interior;
}

LevelData &InteriorWorldData::getActiveLevel()
{
	return this->levels.at(this->levelIndex);
}

const LevelData &InteriorWorldData::getActiveLevel() const
{
	return this->levels.at(this->levelIndex);
}

void InteriorWorldData::setLevelIndex(int levelIndex)
{
	this->levelIndex = levelIndex;
}
