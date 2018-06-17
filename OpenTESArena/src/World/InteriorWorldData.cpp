#include "InteriorWorldData.h"
#include "WorldType.h"
#include "../Math/Random.h"
#include "../Utilities/String.h"

InteriorWorldData::InteriorWorldData()
{
	this->levelIndex = 0;
}

InteriorWorldData::~InteriorWorldData()
{

}

InteriorWorldData InteriorWorldData::loadInterior(const MIFFile &mif, const ExeData &exeData)
{
	InteriorWorldData worldData;

	// Generate levels.
	for (const auto &level : mif.getLevels())
	{
		worldData.levels.push_back(InteriorLevelData::loadInterior(
			level, mif.getDepth(), mif.getWidth(), exeData));
	}

	// Convert start points from the old coordinate system to the new one.
	for (const auto &point : mif.getStartPoints())
	{
		worldData.startPoints.push_back(VoxelGrid::getTransformedCoordinate(
			point, mif.getDepth(), mif.getWidth()));
	}

	worldData.levelIndex = mif.getStartingLevelIndex();
	worldData.mifName = mif.getName();

	return worldData;
}

InteriorWorldData InteriorWorldData::loadDungeon(uint32_t seed, int widthChunks, int depthChunks,
	bool isArtifactDungeon, const ExeData &exeData)
{
	// Load the .MIF file with all the dungeon chunks in it. Dimensions should be 32x32.
	const MIFFile mif("RANDOM1.MIF");

	ArenaRandom random(seed);

	// Number of levels in the dungeon.
	const int levelCount = [isArtifactDungeon, &random]()
	{
		if (isArtifactDungeon)
		{
			return 4;
		}
		else
		{
			return 1 + (random.next() % 2);
		}
	}();

	// Store the seed for later, to be used with block selection.
	const uint32_t seed2 = random.getSeed();

	// Determine transition blocks (*LEVELUP, *LEVELDOWN).
	auto getNextTransBlock = [widthChunks, depthChunks, &random]()
	{
		const int tY = random.next() % depthChunks;
		const int tX = random.next() % widthChunks;
		return (10 * tY) + tX;
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
	const std::string infName = String::toUppercase(mif.getLevels().front().info);

	InteriorWorldData worldData;
	const int gridWidth = mif.getDepth() * depthChunks;
	const int gridDepth = mif.getWidth() * widthChunks;

	// Generate each level, deciding which dungeon blocks to use.
	for (int i = 0; i < levelCount; i++)
	{
		random.srand(seed2 + i);
		const int levelUpBlock = transitions.at(i);

		// No *LEVELDOWN block on the lowest level.
		const int *levelDownBlock = (i < (levelCount - 1)) ? &transitions.at(i + 1) : nullptr;

		worldData.levels.push_back(InteriorLevelData::loadDungeon(
			random, mif.getLevels(), levelUpBlock, levelDownBlock, widthChunks,
			depthChunks, infName, gridWidth, gridDepth, exeData));
	}

	// The start point depends on where the level up voxel is on the first level.
	// Convert it from the old coordinate system to the new one.
	const double chunkDimReal = 32.0;
	const double firstTransitionChunkX = static_cast<double>(transitions.front() % 10);
	const double firstTransitionChunkZ = static_cast<double>(transitions.front() / 10);
	const Double2 startPoint(
		10.50 + (firstTransitionChunkX * chunkDimReal),
		10.50 + (firstTransitionChunkZ * chunkDimReal));
	worldData.startPoints.push_back(VoxelGrid::getTransformedCoordinate(
		startPoint, gridWidth, gridDepth));

	worldData.levelIndex = 0;
	worldData.mifName = mif.getName();

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
