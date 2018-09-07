#include <algorithm>
#include <iomanip>
#include <sstream>

#include "ExteriorLevelData.h"
#include "WorldType.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/Location.h"
#include "../World/LocationType.h"
#include "../World/VoxelDataType.h"

ExteriorLevelData::ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth,
	const std::string &infName, const std::string &name)
	: LevelData(gridWidth, gridHeight, gridDepth, infName, name) { }

ExteriorLevelData::~ExteriorLevelData()
{

}

void ExteriorLevelData::generateBuildingNames(int localCityID, int provinceID, uint32_t citySeed,
	ArenaRandom &random, bool isCoastal, bool isCity, int gridWidth, int gridDepth,
	const MiscAssets &miscAssets)
{
	const auto &exeData = miscAssets.getExeData();
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);

	std::vector<int> seen;
	auto hashInSeen = [&seen](int hash)
	{
		return std::find(seen.begin(), seen.end(), hash) != seen.end();
	};

	auto generateName = [this, localCityID, provinceID, &citySeed, &random, isCoastal, isCity,
		gridWidth, gridDepth, &miscAssets, &exeData, globalCityID, &seen, &hashInSeen](
			int x, int z, VoxelData::WallData::MenuType menuType)
	{
		if ((menuType == VoxelData::WallData::MenuType::Equipment) ||
			(menuType == VoxelData::WallData::MenuType::Temple))
		{
			// X and Y coordinates are swapped and reversed because the modern coordinate system
			// is being used here since it's operating on the resulting voxel grid to avoid
			// decoding voxel bits twice.
			citySeed = (((gridWidth - 1) - x) << 16) + ((gridDepth - 1) - z);
			random.srand(citySeed);
		}

		// Lambdas for creating tavern, equipment store, and temple building names.
		auto createTavernName = [isCoastal, &exeData](int m, int n)
		{
			const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
			const auto &tavernSuffixes = isCoastal ?
				exeData.cityGen.tavernMarineSuffixes : exeData.cityGen.tavernSuffixes;
			return tavernPrefixes.at(m) + ' ' + tavernSuffixes.at(n);
		};

		auto createEquipmentName = [localCityID, provinceID, &random, &miscAssets,
			&exeData, x, z](int m, int n)
		{
			const auto &equipmentPrefixes = exeData.cityGen.equipmentPrefixes;
			const auto &equipmentSuffixes = exeData.cityGen.equipmentSuffixes;

			// Equipment store names can have variables in them.
			std::string str = equipmentPrefixes.at(m) + ' ' + equipmentSuffixes.at(n);

			// Replace %ct with city type string.
			const std::string &cityTypeStr = [localCityID, &exeData]() -> const std::string&
			{
				const int index = [localCityID]()
				{
					const LocationType locationType = Location::getCityType(localCityID);
					if (locationType == LocationType::CityState)
					{
						return 0;
					}
					else if (locationType == LocationType::Town)
					{
						return 1;
					}
					else if (locationType == LocationType::Village)
					{
						return 2;
					}
					else
					{
						throw DebugException("Invalid local city ID \"" +
							std::to_string(localCityID) + "\".");
					}
				}();

				return exeData.locations.locationTypes.at(index);
			}();

			size_t index = str.find("%ct");
			if (index != std::string::npos)
			{
				str.replace(index, 3, cityTypeStr);
			}

			// Replace %ef with generated male first name from (y<<16)+x seed.
			random.srand((x << 16) + z);
			const bool isMale = true;
			const std::string maleFirstName = [&miscAssets, provinceID, isMale, &random]()
			{
				const std::string name = miscAssets.generateNpcName(provinceID, isMale, random);
				const std::string firstName = String::split(name).front();
				return firstName;
			}();

			index = str.find("%ef");
			if (index != std::string::npos)
			{
				str.replace(index, 3, maleFirstName);
			}

			// Replace %n with generated male name from (x<<16)+y seed.
			random.srand((z << 16) + x);
			const std::string maleName = miscAssets.generateNpcName(provinceID, isMale, random);
			index = str.find("%n");
			if (index != std::string::npos)
			{
				str.replace(index, 2, maleName);
			}

			return str;
		};

		auto createTempleName = [&exeData](int model, int n)
		{
			const auto &templePrefixes = exeData.cityGen.templePrefixes;
			const auto &temple1Suffixes = exeData.cityGen.temple1Suffixes;
			const auto &temple2Suffixes = exeData.cityGen.temple2Suffixes;
			const auto &temple3Suffixes = exeData.cityGen.temple3Suffixes;

			const std::string &templeSuffix = [&temple1Suffixes, &temple2Suffixes,
				&temple3Suffixes, model, n]() -> const std::string&
			{
				if (model == 0)
				{
					return temple1Suffixes.at(n);
				}
				else if (model == 1)
				{
					return temple2Suffixes.at(n);
				}
				else
				{
					return temple3Suffixes.at(n);
				}
			}();

			// No extra whitespace needed, I think?
			return templePrefixes.at(model) + templeSuffix;
		};

		// See if the current voxel is a *MENU block and matches the target menu type.
		const bool matchesTargetType = [this, isCity, x, z, menuType]()
		{
			const auto &voxelGrid = this->getVoxelGrid();
			const uint16_t voxelID = voxelGrid.getVoxel(x, 1, z);
			const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
			return (voxelData.dataType == VoxelDataType::Wall) && voxelData.wall.isMenu() &&
				(VoxelData::WallData::getMenuType(voxelData.wall.menuID, isCity) == menuType);
		}();

		if (matchesTargetType)
		{
			// Get the *MENU block's display name.
			int hash;
			std::string name;

			if (menuType == VoxelData::WallData::MenuType::Tavern)
			{
				// Tavern.
				int m, n;
				do
				{
					m = random.next() % 23;
					n = random.next() % 23;
					hash = (m << 8) + n;
				} while (hashInSeen(hash));

				name = createTavernName(m, n);
			}
			else if (menuType == VoxelData::WallData::MenuType::Equipment)
			{
				// Equipment store.
				int m, n;
				do
				{
					m = random.next() % 20;
					n = random.next() % 10;
					hash = (m << 8) + n;
				} while (hashInSeen(hash));

				name = createEquipmentName(m, n);
			}
			else
			{
				// Temple.
				int model, n;
				do
				{
					model = random.next() % 3;
					const std::array<int, 3> ModelVars = { 5, 9, 10 };
					const int vars = ModelVars.at(model);
					n = random.next() % vars;
					hash = (model << 8) + n;
				} while (hashInSeen(hash));

				name = createTempleName(model, n);
			}

			this->menuNames.push_back(std::make_pair(Int2(x, z), std::move(name)));
			seen.push_back(hash);
		}

		// Fix some edge cases presumably caught during Arena's play-testing.
		if ((menuType == VoxelData::WallData::MenuType::Temple) &&
			((globalCityID == 2) || (globalCityID == 0xE0)))
		{
			int model, n;
			if (globalCityID == 2)
			{
				model = 1;
				n = 7;
			}
			else
			{
				model = 2;
				n = 8;
			}

			this->menuNames.back().second = createTempleName(model, n);
		}
	};

	auto doGenerationLoop = [gridWidth, gridDepth, &generateName](
		VoxelData::WallData::MenuType menuType)
	{
		for (int x = gridWidth - 1; x >= 0; x--)
		{
			for (int z = gridDepth - 1; z >= 0; z--)
			{
				generateName(x, z, menuType);
			}
		}
	};

	doGenerationLoop(VoxelData::WallData::MenuType::Tavern);
	doGenerationLoop(VoxelData::WallData::MenuType::Equipment);
	doGenerationLoop(VoxelData::WallData::MenuType::Temple);
}

ExteriorLevelData ExteriorLevelData::loadPremadeCity(const MIFFile::Level &level,
	const std::string &infName, int gridWidth, int gridDepth, const MiscAssets &miscAssets)
{
	// Premade exterior level (only used by center province).
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.name);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	const auto &exeData = miscAssets.getExeData();
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1.data(), inf, WorldType::City, gridWidth, gridDepth, exeData);
	levelData.readMAP2(level.map2.data(), inf, gridWidth, gridDepth);

	// Generate building names.
	// @todo: pass these as arguments to loadPremadeCity() instead of hardcoding them.
	const auto &cityData = miscAssets.getCityDataFile();
	const int localCityID = 0;
	const int provinceID = 8;
	const uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);
	ArenaRandom random(citySeed);
	const bool isCoastal = false;
	const bool isCity = true;
	levelData.generateBuildingNames(localCityID, provinceID, citySeed, random, isCoastal,
		isCity, gridWidth, gridDepth, miscAssets);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadCity(const MIFFile::Level &level, int localCityID,
	int provinceID, int cityDim, bool isCoastal, const std::vector<uint8_t> &reservedBlocks,
	const Int2 &startPosition, const std::string &infName, int gridWidth, int gridDepth,
	const MiscAssets &miscAssets)
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

	// Get the city's seed for random chunk generation. It is modified later during
	// building name generation.
	const auto &cityData = miscAssets.getCityDataFile();
	uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);

	// Get the city's local X and Y, to be used later for building name generation.
	const Int2 localCityPoint = CityDataFile::getLocalCityPoint(citySeed);

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
	const auto &exeData = miscAssets.getExeData();
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, WorldType::City, gridWidth, gridDepth, exeData);
	levelData.readMAP2(tempMap2.data(), inf, gridWidth, gridDepth);

	// Generate building names.
	const bool isCity = true;
	levelData.generateBuildingNames(localCityID, provinceID, citySeed, random, isCoastal,
		isCity, gridWidth, gridDepth, miscAssets);

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

const std::vector<std::pair<Int2, std::string>> &ExteriorLevelData::getMenuNames() const
{
	return this->menuNames;
}

bool ExteriorLevelData::isOutdoorDungeon() const
{
	return false;
}
