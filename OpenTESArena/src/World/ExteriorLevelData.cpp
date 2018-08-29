#include <algorithm>
#include <iomanip>
#include <sstream>

#include "ExteriorLevelData.h"
#include "WorldType.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"

ExteriorLevelData::ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth,
	const std::string &infName, const std::string &name)
	: LevelData(gridWidth, gridHeight, gridDepth, infName, name) { }

ExteriorLevelData::~ExteriorLevelData()
{

}

ExteriorLevelData ExteriorLevelData::loadPremadeCity(const MIFFile::Level &level,
	const std::string &infName, int gridWidth, int gridDepth, const ExeData &exeData)
{
	// Premade exterior level (only used by center province).
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.name);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1.data(), inf, WorldType::City, gridWidth, gridDepth, exeData);
	levelData.readMAP2(level.map2.data(), inf, gridWidth, gridDepth);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadCity(const MIFFile::Level &level, uint32_t citySeed,
	int cityDim, const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
	const std::string &infName, int gridWidth, int gridDepth, const ExeData &exeData)
{
	// Create temp voxel data buffers and write the city skeleton data to them. Each city
	// block will be written to them as well.
	std::vector<uint16_t> tempFlor(level.flor.begin(), level.flor.end());
	std::vector<uint16_t> tempMap1(level.map1.begin(), level.map1.end());
	std::vector<uint16_t> tempMap2(level.map2.begin(), level.map2.end());

	// Decide which city blocks to load.
	enum class BlockType
	{
		Empty, Reserved, Equipment, MagesGuild,
		NobleHouse, Temple, Tavern, Spacer, Houses
	};

	const int citySize = cityDim * cityDim;
	std::vector<BlockType> plan(citySize, BlockType::Empty);
	ArenaRandom random(citySeed);

	auto placeBlock = [citySize, &plan, &random](BlockType blockType)
	{
		int planIndex;

		do
		{
			planIndex = random.next() % citySize;
		} while (plan.at(planIndex) != BlockType::Empty);

		plan.at(planIndex) = blockType;
	};

	// Set reserved blocks.
	for (const uint8_t block : reservedBlocks)
	{
		// The original engine uses a fixed array so all block indices always fall within the
		// plan, but since a dynamic array is used here, it has to ignore out-of-bounds blocks
		// explicitly.
		if (block < plan.size())
		{
			plan.at(block) = BlockType::Reserved;
		}
	}

	// Initial block placement.
	placeBlock(BlockType::Equipment);
	placeBlock(BlockType::MagesGuild);
	placeBlock(BlockType::NobleHouse);
	placeBlock(BlockType::Temple);
	placeBlock(BlockType::Tavern);
	placeBlock(BlockType::Spacer);

	// Create city plan according to RNG.
	const int emptyBlocksInPlan = static_cast<int>(
		std::count(plan.begin(), plan.end(), BlockType::Empty));
	for (int remainingBlocks = emptyBlocksInPlan; remainingBlocks > 0; remainingBlocks--)
	{
		const uint32_t randVal = random.next();
		const BlockType blockType = [randVal]()
		{
			if (randVal <= 0x7333)
			{
				return BlockType::Houses;
			}
			else if (randVal <= 0xA666)
			{
				return BlockType::Tavern;
			}
			else if (randVal <= 0xCCCC)
			{
				return BlockType::Equipment;
			}
			else if (randVal <= 0xE666)
			{
				return BlockType::Temple;
			}
			else
			{
				return BlockType::NobleHouse;
			}
		}();

		placeBlock(blockType);
	}

	// Build the city, loading data for each block. Load blocks right to left, top to bottom.
	int xDim = 0;
	int yDim = 0;

	for (const BlockType block : plan)
	{
		if (block != BlockType::Reserved)
		{
			const std::array<std::string, 7> BlockCodes =
			{
				"EQ", "MG", "NB", "TP", "TV", "TS", "BS"
			};

			const std::array<int, 7> VariationCounts =
			{
				13, 11, 10, 12, 15, 11, 20
			};

			const std::array<std::string, 4> Rotations =
			{
				"A", "B", "C", "D"
			};

			const int blockIndex = static_cast<int>(block) - 2;
			const std::string &blockCode = BlockCodes.at(blockIndex);
			const std::string &rotation = Rotations.at(random.next() % Rotations.size());
			const int variationCount = VariationCounts.at(blockIndex);
			const int variation = std::max(random.next() % variationCount, 1);
			const std::string blockMifName = blockCode + "BD" +
				std::to_string(variation) + rotation + ".MIF";

			// Load the block's .MIF data into the level.
			const MIFFile blockMif(blockMifName);
			const auto &blockLevel = blockMif.getLevels().front();

			// Offset of the block in the voxel grid.
			const int xOffset = startPosition.x + (xDim * 20);
			const int zOffset = startPosition.y + (yDim * 20);

			// Copy block data to temp buffers.
			for (int z = 0; z < blockMif.getDepth(); z++)
			{
				const int srcIndex = z * blockMif.getWidth();
				const int dstIndex = xOffset + ((z + zOffset) * gridDepth);

				auto writeRow = [&blockMif, srcIndex, dstIndex](
					const std::vector<uint16_t> &src, std::vector<uint16_t> &dst)
				{
					const auto srcBegin = src.begin() + srcIndex;
					const auto srcEnd = srcBegin + blockMif.getWidth();
					const auto dstBegin = dst.begin() + dstIndex;
					std::copy(srcBegin, srcEnd, dstBegin);
				};

				writeRow(blockLevel.flor, tempFlor);
				writeRow(blockLevel.map1, tempMap1);
				writeRow(blockLevel.map2, tempMap2);
			}

			// @todo: load flats.
		}

		xDim++;

		// Move to the next row if done with the current one.
		if (xDim == cityDim)
		{
			xDim = 0;
			yDim++;
		}
	}

	// Create the level for the voxel data to be written into.
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.name);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, WorldType::City, gridWidth, gridDepth, exeData);
	levelData.readMAP2(tempMap2.data(), inf, gridWidth, gridDepth);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL,
	const std::string &infName, const ExeData &exeData)
{
	// Load WILD.MIF (blank slate, to be filled in by four .RMD files).
	const MIFFile mif("WILD.MIF");
	const MIFFile::Level &level = mif.getLevels().front();
	const int gridWidth = 128;
	const int gridDepth = gridWidth;

	// Copy voxel data into temp buffers. Each floor in the four 64x64 wilderness blocks
	// is 8192 bytes. Using vectors here because arrays are too large on the stack.
	std::vector<uint16_t> tempFlor(RMDFile::ELEMENTS_PER_FLOOR * 4);
	std::vector<uint16_t> tempMap1(tempFlor.size());
	std::vector<uint16_t> tempMap2(tempMap1.size());
	std::copy(level.flor.begin(), level.flor.end(), tempFlor.begin());
	std::copy(level.map1.begin(), level.map1.end(), tempMap1.begin());
	std::copy(level.map2.begin(), level.map2.end(), tempMap2.begin());

	auto writeRMD = [gridDepth, &tempFlor, &tempMap1, &tempMap2](
		int rmdID, int xOffset, int zOffset)
	{
		const std::string rmdName = [rmdID]()
		{
			std::stringstream ss;
			ss << std::setw(3) << std::setfill('0') << rmdID;
			return "WILD" + ss.str() + ".RMD";
		}();

		const RMDFile rmd(rmdName);

		// Copy .RMD voxel data to temp buffers.
		for (int z = 0; z < RMDFile::DEPTH; z++)
		{
			const int srcIndex = z * RMDFile::WIDTH;
			const int dstIndex = xOffset + ((z + zOffset) * gridDepth);

			auto writeRow = [srcIndex, dstIndex](const std::vector<uint16_t> &src,
				std::vector<uint16_t> &dst)
			{
				const auto srcBegin = src.begin() + srcIndex;
				const auto srcEnd = srcBegin + RMDFile::WIDTH;
				const auto dstBegin = dst.begin() + dstIndex;
				std::copy(srcBegin, srcEnd, dstBegin);
			};

			writeRow(rmd.getFLOR(), tempFlor);
			writeRow(rmd.getMAP1(), tempMap1);
			writeRow(rmd.getMAP2(), tempMap2);
		}
	};

	// Load four .RMD files into the wilderness skeleton, each at some X and Z offset in
	// the voxel grid.
	writeRMD(rmdTR, 0, 0); // Top right.
	writeRMD(rmdTL, RMDFile::WIDTH, 0); // Top left.
	writeRMD(rmdBR, 0, RMDFile::DEPTH); // Bottom right.
	writeRMD(rmdBL, RMDFile::WIDTH, RMDFile::DEPTH); // Bottom left.

	// Create the level for the voxel data to be written into.
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.name);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, WorldType::Wilderness, gridWidth, gridDepth, exeData);
	levelData.readMAP2(tempMap2.data(), inf, gridWidth, gridDepth);
	// @todo: load FLAT from WILD.MIF level data. levelData.readFLAT(level.flat, ...)?

	return levelData;
}

bool ExteriorLevelData::isOutdoorDungeon() const
{
	return false;
}
