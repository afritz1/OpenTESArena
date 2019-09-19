#include <algorithm>
#include <iomanip>

#include "ExteriorLevelData.h"
#include "WorldType.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../World/Location.h"
#include "../World/LocationType.h"
#include "../World/VoxelDataType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"

ExteriorLevelData::ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth,
	const std::string &infName, const std::string &name)
	: LevelData(gridWidth, gridHeight, gridDepth, infName, name) { }

ExteriorLevelData::~ExteriorLevelData()
{

}

void ExteriorLevelData::generateCity(int localCityID, int provinceID, int cityDim, int gridDepth,
	const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition, uint32_t citySeed,
	ArenaRandom &random, std::vector<uint16_t> &dstFlor, std::vector<uint16_t> &dstMap1,
	std::vector<uint16_t> &dstMap2)
{
	// Decide which city blocks to load.
	enum class BlockType
	{
		Empty, Reserved, Equipment, MagesGuild,
		NobleHouse, Temple, Tavern, Spacer, Houses
	};

	// Get the city's local X and Y, to be used later for building name generation.
	const Int2 localCityPoint = CityDataFile::getLocalCityPoint(citySeed);

	const int citySize = cityDim * cityDim;
	std::vector<BlockType> plan(citySize, BlockType::Empty);

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
			MIFFile blockMif;
			if (!blockMif.init(blockMifName.c_str()))
			{
				DebugCrash("Could not init .MIF file \"" + blockMifName + "\".");
			}

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

				writeRow(blockLevel.flor, dstFlor);
				writeRow(blockLevel.map1, dstMap1);
				writeRow(blockLevel.map2, dstMap2);
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
}

void ExteriorLevelData::generateBuildingNames(int localCityID, int provinceID, uint32_t citySeed,
	ArenaRandom &random, bool isCoastal, bool isCity, int gridWidth, int gridDepth,
	const MiscAssets &miscAssets)
{
	const auto &exeData = miscAssets.getExeData();
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);
	const Int2 localCityPoint = CityDataFile::getLocalCityPoint(citySeed);

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [this, localCityID, provinceID, &citySeed, &random, isCoastal, isCity,
		gridWidth, gridDepth, &miscAssets, &exeData, globalCityID, &localCityPoint](
			VoxelData::WallData::MenuType menuType)
	{
		if ((menuType == VoxelData::WallData::MenuType::Equipment) ||
			(menuType == VoxelData::WallData::MenuType::Temple))
		{
			citySeed = (localCityPoint.x << 16) + localCityPoint.y;
			random.srand(citySeed);
		}

		std::vector<int> seen;
		auto hashInSeen = [&seen](int hash)
		{
			return std::find(seen.begin(), seen.end(), hash) != seen.end();
		};

		// Lambdas for creating tavern, equipment store, and temple building names.
		auto createTavernName = [isCoastal, &exeData](int m, int n)
		{
			const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
			const auto &tavernSuffixes = isCoastal ?
				exeData.cityGen.tavernMarineSuffixes : exeData.cityGen.tavernSuffixes;
			return tavernPrefixes.at(m) + ' ' + tavernSuffixes.at(n);
		};

		auto createEquipmentName = [localCityID, provinceID, &random, gridWidth, gridDepth,
			&miscAssets, &exeData](int m, int n, int x, int z)
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
						DebugUnhandledReturnMsg(int, std::to_string(localCityID));
					}
				}();

				return exeData.locations.locationTypes.at(index);
			}();

			size_t index = str.find("%ct");
			if (index != std::string::npos)
			{
				str.replace(index, 3, cityTypeStr);
			}

			// Replace %ef with generated male first name from (y<<16)+x seed. Use a local RNG for
			// modifications to building names. Swap and reverse the XZ dimensions so they fit the
			// original XY values in Arena.
			index = str.find("%ef");
			if (index != std::string::npos)
			{
				ArenaRandom nameRandom((((gridWidth - 1) - x) << 16) + ((gridDepth - 1) - z));
				const bool isMale = true;
				const std::string maleFirstName = [&miscAssets, provinceID, isMale, &nameRandom]()
				{
					const std::string name = miscAssets.generateNpcName(
						provinceID, isMale, nameRandom);
					const std::string firstName = String::split(name).front();
					return firstName;
				}();

				str.replace(index, 3, maleFirstName);
			}

			// Replace %n with generated male name from (x<<16)+y seed.
			index = str.find("%n");
			if (index != std::string::npos)
			{
				ArenaRandom nameRandom((((gridDepth - 1) - z) << 16) + ((gridWidth - 1) - x));
				const bool isMale = true;
				const std::string maleName = miscAssets.generateNpcName(
					provinceID, isMale, nameRandom);
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

		// The lambda called for each main-floor voxel in the area.
		auto tryGenerateBlockName = [this, isCity, menuType, &random, globalCityID, &seen,
			&hashInSeen, &createTavernName, &createEquipmentName, &createTempleName](int x, int z)
		{
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

					name = createEquipmentName(m, n, x, z);
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
		};

		// Start at the top-right corner of the map, running right to left and top to bottom.
		for (int x = gridWidth - 1; x >= 0; x--)
		{
			for (int z = gridDepth - 1; z >= 0; z--)
			{
				tryGenerateBlockName(x, z);
			}
		}

		// Fix some edge cases used with the main quest.
		if ((menuType == VoxelData::WallData::MenuType::Temple) &&
			((globalCityID == 2) || (globalCityID == 0xE0)))
		{
			// Added an index variable since Arena apparently stores its menu names in a way
			// other than with a vector like this solution is using.
			int model, n, index;
			if (globalCityID == 2)
			{
				model = 1;
				n = 7;
				index = 23;
			}
			else
			{
				model = 2;
				n = 8;
				index = 32;
			}

			this->menuNames.at(index).second = createTempleName(model, n);
		}
	};

	generateNames(VoxelData::WallData::MenuType::Tavern);
	generateNames(VoxelData::WallData::MenuType::Equipment);
	generateNames(VoxelData::WallData::MenuType::Temple);
}

void ExteriorLevelData::generateWildChunkBuildingNames(int localCityID, int provinceID,
	const MiscAssets &miscAssets)
{
	// @todo
	/*const auto &cityData = miscAssets.getCityDataFile();
	const uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);*/
}

void ExteriorLevelData::revisePalaceGraphics(std::vector<uint16_t> &map1, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [&map1, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map1.data()) + index);
		return voxel;
	};

	auto setMap1Voxel = [&map1, gridWidth, gridDepth](int x, int z, uint16_t voxel)
	{
		// Set voxel data in reverse order.
		const int index = ((gridDepth - 1) - z) + (((gridWidth - 1) - x) * gridDepth);
		map1.at(index) = voxel;
	};

	struct SearchResult
	{
		enum class Side { None, North, South, East, West };

		Side side;

		// Distance from the associated origin dimension, where (0, 0) is at the bottom left.
		int offset;

		SearchResult(Side side, int offset)
		{
			this->side = side;
			this->offset = offset;
		}
	};

	// Find one of the palace graphic blocks, then extrapolate the positions of
	// the other palace graphic and the gates.
	const SearchResult result = [gridWidth, gridDepth, &getMap1Voxel]()
	{
		auto isPalaceBlock = [&getMap1Voxel](int x, int z)
		{
			const uint16_t voxel = getMap1Voxel(x, z);
			const uint8_t mostSigNibble = (voxel & 0xF000) >> 12;
			return mostSigNibble == 0x9;
		};

		// North (top edge) and south (bottom edge), search left to right.
		for (int z = 1; z < (gridDepth - 1); z++)
		{
			const int northX = gridWidth - 1;
			const int southX = 0;
			if (isPalaceBlock(northX, z))
			{
				return SearchResult(SearchResult::Side::North, z);
			}
			else if (isPalaceBlock(southX, z))
			{
				return SearchResult(SearchResult::Side::South, z);
			}
		}

		// East (right edge) and west (left edge), search bottom to top.
		for (int x = 1; x < (gridWidth - 1); x++)
		{
			const int eastZ = gridDepth - 1;
			const int westZ = 0;
			if (isPalaceBlock(x, eastZ))
			{
				return SearchResult(SearchResult::Side::East, x);
			}
			else if (isPalaceBlock(x, westZ))
			{
				return SearchResult(SearchResult::Side::West, x);
			}
		}

		// No palace gate found. This should never happen because every city/town/village
		// in the original game has a palace gate somewhere.
		return SearchResult(SearchResult::Side::None, 0);
	}();

	// Decide how to extrapolate the search results.
	if (result.side != SearchResult::Side::None)
	{
		// The direction to step from a palace voxel to the other palace voxel.
		const Int2 northSouthPalaceStep(0, 1);
		const Int2 eastWestPalaceStep(1, 0);

		// Gets the distance in voxels from a palace voxel to its gate, or -1 if no gate exists.
		const int NO_GATE = -1;
		auto getGateDistance = [&getMap1Voxel, NO_GATE](const Int2 &palaceVoxel, const Int2 &dir)
		{
			auto isGateBlock = [&getMap1Voxel](int x, int z)
			{
				const uint16_t voxel = getMap1Voxel(x, z);
				const uint8_t mostSigNibble = (voxel & 0xF000) >> 12;
				return mostSigNibble == 0xA;
			};

			// Gates should usually be within a couple blocks of their castle graphic. If not,
			// then no gate exists.
			const int MAX_GATE_DIST = 8;

			int i = 0;
			Int2 position = palaceVoxel;
			while ((i < MAX_GATE_DIST) && !isGateBlock(position.x, position.y))
			{
				position = position + dir;
				i++;
			}

			return (i < MAX_GATE_DIST) ? i : NO_GATE;
		};

		// Set the positions of the two palace voxels and the two gate voxels.
		Int2 firstPalaceVoxel, secondPalaceVoxel, firstGateVoxel, secondGateVoxel;
		uint16_t firstPalaceVoxelID, secondPalaceVoxelID, gateVoxelID;
		int gateDist;
		if (result.side == SearchResult::Side::North)
		{
			firstPalaceVoxel = Int2(gridWidth - 1, result.offset);
			secondPalaceVoxel = firstPalaceVoxel + northSouthPalaceStep;
			const Int2 gateDir = Int2(-1, 0);
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + northSouthPalaceStep;
			firstPalaceVoxelID = 0xA5B5;
			secondPalaceVoxelID = 0xA5B4;
			gateVoxelID = 0xA1B3;
		}
		else if (result.side == SearchResult::Side::South)
		{
			firstPalaceVoxel = Int2(0, result.offset);
			secondPalaceVoxel = firstPalaceVoxel + northSouthPalaceStep;
			const Int2 gateDir = Int2(1, 0);
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + northSouthPalaceStep;
			firstPalaceVoxelID = 0xA534;
			secondPalaceVoxelID = 0xA535;
			gateVoxelID = 0xA133;
		}
		else if (result.side == SearchResult::Side::East)
		{
			firstPalaceVoxel = Int2(result.offset, gridDepth - 1);
			secondPalaceVoxel = firstPalaceVoxel + eastWestPalaceStep;
			const Int2 gateDir = Int2(0, -1);
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + eastWestPalaceStep;
			firstPalaceVoxelID = 0xA574;
			secondPalaceVoxelID = 0xA575;
			gateVoxelID = 0xA173;
		}
		else if (result.side == SearchResult::Side::West)
		{
			firstPalaceVoxel = Int2(result.offset, 0);
			secondPalaceVoxel = firstPalaceVoxel + eastWestPalaceStep;
			const Int2 gateDir = Int2(0, 1);
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + eastWestPalaceStep;
			firstPalaceVoxelID = 0xA5F5;
			secondPalaceVoxelID = 0xA5F4;
			gateVoxelID = 0xA1F3;
		}

		// Set the voxel IDs to their new values.
		setMap1Voxel(firstPalaceVoxel.x, firstPalaceVoxel.y, firstPalaceVoxelID);
		setMap1Voxel(secondPalaceVoxel.x, secondPalaceVoxel.y, secondPalaceVoxelID);

		if (gateDist != NO_GATE)
		{
			setMap1Voxel(firstGateVoxel.x, firstGateVoxel.y, gateVoxelID);
			setMap1Voxel(secondGateVoxel.x, secondGateVoxel.y, gateVoxelID);
		}
	}
	else
	{
		// The search did not find any palace graphics block.
		DebugLogWarning("No palace graphics found to revise.");
	}
}

Buffer2D<uint8_t> ExteriorLevelData::generateWildernessIndices(uint32_t wildSeed,
	const ExeData::Wilderness &wildData)
{
	const int wildWidth = 64;
	const int wildHeight = 64;
	Buffer2D<uint8_t> indices(wildWidth, wildHeight);
	ArenaRandom random(wildSeed);

	// Generate a random wilderness .MIF index for each wilderness chunk.
	std::generate(indices.get(), indices.get() + (indices.getWidth() * indices.getHeight()),
		[&wildData, &random]()
	{
		// Determine the wilderness block list to draw from.
		const auto &blockList = [&wildData, &random]() -> const std::vector<uint8_t>&
		{
			const uint16_t normalVal = 0x6666;
			const uint16_t villageVal = 0x4000;
			const uint16_t dungeonVal = 0x2666;
			const uint16_t tavernVal = 0x1999;
			int randVal = random.next();

			if (randVal < normalVal)
			{
				return wildData.normalBlocks;
			}
			else
			{
				randVal -= normalVal;
				if (randVal < villageVal)
				{
					return wildData.villageBlocks;
				}
				else
				{
					randVal -= villageVal;
					if (randVal < dungeonVal)
					{
						return wildData.dungeonBlocks;
					}
					else
					{
						randVal -= dungeonVal;
						if (randVal < tavernVal)
						{
							return wildData.tavernBlocks;
						}
						else
						{
							return wildData.templeBlocks;
						}
					}
				}
			}
		}();

		const int blockListIndex = (random.next() & 0xFF) % blockList.size();
		return blockList[blockListIndex];
	});

	// City indices in the center of the wilderness (WILD001.MIF, etc.).
	DebugAssertMsg(wildWidth >= 2, "Wild width \"" + std::to_string(wildWidth) + "\" too small.");
	DebugAssertMsg(wildHeight >= 2, "Wild height \"" + std::to_string(wildHeight) + "\" too small.");
	const int cityX = (wildWidth / 2) - 1;
	const int cityY = (wildHeight / 2) - 1;
	indices.set(cityX, cityY, 1);
	indices.set(cityX + 1, cityY, 2);
	indices.set(cityX, cityY + 1, 3);
	indices.set(cityX + 1, cityY + 1, 4);

	return indices;
}

void ExteriorLevelData::reviseWildernessCity(int localCityID, int provinceID,
	Buffer2D<uint16_t> &flor, Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2,
	const MiscAssets &miscAssets)
{
	// For now, assume the given buffers are for the entire 4096x4096 wilderness.
	// @todo: change to only care about 128x128 layers.
	DebugAssert(flor.getWidth() == (64 * RMDFile::WIDTH));
	DebugAssert(flor.getWidth() == flor.getHeight());
	DebugAssert(flor.getWidth() == map1.getWidth());
	DebugAssert(flor.getWidth() == map2.getWidth());

	// Clear all placeholder city blocks.
	const int placeholderWidth = RMDFile::WIDTH * 2;
	const int placeholderDepth = RMDFile::DEPTH * 2;

	// @todo: change to only care about 128x128 floors -- these should both be removed.
	const int xOffset = RMDFile::WIDTH * 31;
	const int zOffset = RMDFile::DEPTH * 31;

	for (int x = 0; x < placeholderWidth; x++)
	{
		const int startIndex = zOffset + ((x + xOffset) * flor.getWidth());

		auto clearRow = [placeholderDepth, startIndex](Buffer2D<uint16_t> &dst)
		{
			const auto dstBegin = dst.get() + startIndex;
			const auto dstEnd = dstBegin + placeholderDepth;
			DebugAssert(dstEnd <= dst.end());
			std::fill(dstBegin, dstEnd, 0);
		};

		clearRow(flor);
		clearRow(map1);
		clearRow(map2);
	}

	// Get city generation info.
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);
	DebugAssertMsg((globalCityID >= 0) && (globalCityID <= 256),
		"Invalid city ID \"" + std::to_string(globalCityID) + "\".");

	// Determine city traits from the given city ID.
	const auto &exeData = miscAssets.getExeData();
	const auto &cityGen = exeData.cityGen;
	const LocationType locationType = Location::getCityType(localCityID);
	const bool isCityState = locationType == LocationType::CityState;
	const bool isCoastal = std::find(cityGen.coastalCityList.begin(),
		cityGen.coastalCityList.end(), globalCityID) != cityGen.coastalCityList.end();
	const int templateCount = CityDataFile::getCityTemplateCount(isCoastal, isCityState);
	const int templateID = globalCityID % templateCount;

	const std::string mifName = [provinceID, &exeData, &cityGen, locationType,
		isCoastal, templateID]()
	{
		// Special case for center province.
		if (provinceID == Location::CENTER_PROVINCE_ID)
		{
			return String::toUppercase(exeData.locations.centerProvinceCityMifName);
		}
		else
		{
			// Get the index into the template names array (town%d.mif, ..., cityw%d.mif).
			const int nameIndex = CityDataFile::getCityTemplateNameIndex(locationType, isCoastal);

			// Get the template name associated with the city ID.
			std::string templateName = cityGen.templateFilenames.at(nameIndex);
			templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
			templateName = String::toUppercase(templateName);

			return templateName;
		}
	}();

	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return;
	}

	const MIFFile::Level &level = mif.getLevels().front();

	// Buffers for the city data. Copy the .MIF data into them.
	std::vector<uint16_t> cityFlor(level.flor.begin(), level.flor.end());
	std::vector<uint16_t> cityMap1(level.map1.begin(), level.map1.end());
	std::vector<uint16_t> cityMap2(level.map2.begin(), level.map2.end());

	// Run city generation if it's not the center province. The center province does not have
	// any special generation -- the .MIF buffers are simply used as-is (with some simple palace
	// gate revisions done afterwards).
	if (provinceID != Location::CENTER_PROVINCE_ID)
	{
		// City block count (6x6, 5x5, 4x4).
		const int cityDim = CityDataFile::getCityDimensions(locationType);

		// Get the reserved block list for the given city.
		const std::vector<uint8_t> &reservedBlocks = [&cityGen, isCoastal, templateID]()
		{
			const int index = CityDataFile::getCityReservedBlockListIndex(isCoastal, templateID);
			return cityGen.reservedBlockLists.at(index);
		}();

		// Get the starting position of city blocks within the city skeleton.
		const Int2 startPosition = [locationType, &cityGen, isCoastal, templateID]()
		{
			const int index = CityDataFile::getCityStartingPositionIndex(
				locationType, isCoastal, templateID);

			const auto &pair = cityGen.startingPositions.at(index);
			return Int2(pair.first, pair.second);
		}();

		const auto &cityData = miscAssets.getCityDataFile();
		const uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);
		ArenaRandom random(citySeed);

		// Write generated city data into the temp city buffers.
		ExteriorLevelData::generateCity(localCityID, provinceID, cityDim, mif.getWidth(),
			reservedBlocks, startPosition, citySeed, random, cityFlor, cityMap1, cityMap2);
	}

	// Transform city voxels based on the wilderness rules.
	for (int x = 0; x < mif.getWidth(); x++)
	{
		for (int z = 0; z < mif.getDepth(); z++)
		{
			const int index = DebugMakeIndex(cityFlor, z + (x * mif.getDepth()));
			uint16_t &map1Voxel = cityMap1[index];
			uint16_t &map2Voxel = cityMap2[index];

			if ((map1Voxel & 0x8000) != 0)
			{
				map1Voxel = 0;
			}
			else
			{
				const bool isWall = (map1Voxel == 0x2F2F) || (map1Voxel == 0x2D2D) || (map1Voxel == 0x2E2E);
				if (!isWall)
				{
					map1Voxel = 0;
				}
				else
				{
					// Replace solid walls.
					if ((map1Voxel & 0xFFFF) == 0x2F2F)
					{
						map1Voxel = 0x3030;
					}
					else if ((map1Voxel & 0xFFFF) == 0x2D2D)
					{
						map1Voxel = 0x2F2F;
					}
				}
			}

			// @todo: need to fix second-story voxels.
			if (((map2Voxel & 0xF000) == 0xA000) || (map2Voxel == 0x2F2F))
			{
				// @todo: replace type 0xA with 0xB?
				map2Voxel = 0x0030 | (map2Voxel & 0x8080);
			}
			else
			{
				map2Voxel = 0;
			}
		}
	}

	// Write city buffers into the wilderness.
	for (int z = 0; z < mif.getDepth(); z++)
	{
		const int srcIndex = DebugMakeIndex(cityFlor, z * mif.getWidth());
		const int dstIndex = xOffset + ((z + zOffset) * flor.getWidth());

		auto writeRow = [&mif, srcIndex, dstIndex](
			const std::vector<uint16_t> &src, Buffer2D<uint16_t> &dst)
		{
			const auto srcBegin = src.begin() + srcIndex;
			const auto srcEnd = srcBegin + mif.getWidth();
			const auto dstBegin = dst.get() + dstIndex;
			DebugAssert((dstBegin + std::distance(srcBegin, srcEnd)) <= dst.end());
			std::copy(srcBegin, srcEnd, dstBegin);
		};

		writeRow(cityFlor, flor);
		writeRow(cityMap1, map1);
		writeRow(cityMap2, map2);
	}
}

Int2 ExteriorLevelData::getRelativeWildOrigin(const Int2 &voxel)
{
	return Int2(
		voxel.x - (voxel.x % (RMDFile::WIDTH * 2)),
		voxel.y - (voxel.y % (RMDFile::DEPTH * 2)));
}

ExteriorLevelData ExteriorLevelData::loadPremadeCity(const MIFFile::Level &level,
	WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
	int gridWidth, int gridDepth, const MiscAssets &miscAssets, TextureManager &textureManager)
{
	// Load MAP1 into a temporary buffer so we can revise the palace gate graphics.
	std::vector<uint16_t> tempMap1(level.map1.begin(), level.map1.end());
	ExteriorLevelData::revisePalaceGraphics(tempMap1, gridWidth, gridDepth);

	// Premade exterior level (only used by center province).
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.name);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	const auto &exeData = miscAssets.getExeData();
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, WorldType::City, gridWidth, gridDepth, exeData);
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

	// Generate distant sky.
	levelData.distantSky.init(localCityID, provinceID, weatherType, currentDay,
		starCount, miscAssets, textureManager);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadCity(const MIFFile::Level &level, int localCityID,
	int provinceID, WeatherType weatherType, int currentDay, int starCount, int cityDim,
	bool isCoastal, const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
	const std::string &infName, int gridWidth, int gridDepth, const MiscAssets &miscAssets,
	TextureManager &textureManager)
{
	// Create temp voxel data buffers and write the city skeleton data to them. Each city
	// block will be written to them as well.
	std::vector<uint16_t> tempFlor(level.flor.begin(), level.flor.end());
	std::vector<uint16_t> tempMap1(level.map1.begin(), level.map1.end());
	std::vector<uint16_t> tempMap2(level.map2.begin(), level.map2.end());

	// Get the city's seed for random chunk generation. It is modified later during
	// building name generation.
	const auto &cityData = miscAssets.getCityDataFile();
	const uint32_t citySeed = cityData.getCitySeed(localCityID, provinceID);
	ArenaRandom random(citySeed);
	
	// Generate the bulk of city data and write it into the temp buffers.
	ExteriorLevelData::generateCity(localCityID, provinceID, cityDim, gridDepth, reservedBlocks,
		startPosition, citySeed, random, tempFlor, tempMap1, tempMap2);

	// Run the palace gate graphic algorithm over the perimeter of the MAP1 data.
	ExteriorLevelData::revisePalaceGraphics(tempMap1, gridWidth, gridDepth);

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

	// Generate distant sky.
	levelData.distantSky.init(localCityID, provinceID, weatherType, currentDay,
		starCount, miscAssets, textureManager);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadWilderness(int localCityID, int provinceID,
	WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
	const MiscAssets &miscAssets, TextureManager &textureManager)
{
	const auto &cityData = miscAssets.getCityDataFile();
	const auto &wildData = miscAssets.getExeData().wild;
	const uint32_t wildSeed = cityData.getWildernessSeed(localCityID, provinceID);
	const Buffer2D<uint8_t> wildIndices =
		ExteriorLevelData::generateWildernessIndices(wildSeed, wildData);

	// Temp buffers for voxel data.
	Buffer2D<uint16_t> tempFlor(RMDFile::DEPTH * wildIndices.getWidth(),
		RMDFile::WIDTH * wildIndices.getHeight());
	Buffer2D<uint16_t> tempMap1(tempFlor.getWidth(), tempFlor.getHeight());
	Buffer2D<uint16_t> tempMap2(tempFlor.getWidth(), tempFlor.getHeight());
	tempFlor.fill(0);
	tempMap1.fill(0);
	tempMap2.fill(0);

	auto writeRMD = [&miscAssets, &tempFlor, &tempMap1, &tempMap2](
		uint8_t rmdID, int xOffset, int zOffset)
	{
		const std::vector<RMDFile> &rmdFiles = miscAssets.getWildernessChunks();
		const int rmdIndex = DebugMakeIndex(rmdFiles, rmdID - 1);
		const RMDFile &rmd = rmdFiles[rmdIndex];

		// Copy .RMD voxel data to temp buffers.
		for (int z = 0; z < RMDFile::DEPTH; z++)
		{
			const int srcIndex = z * RMDFile::DEPTH;
			const int dstIndex = xOffset + ((z + zOffset) * tempFlor.getWidth());

			auto writeRow = [srcIndex, dstIndex](const std::vector<uint16_t> &src,
				Buffer2D<uint16_t> &dst)
			{
				const auto srcBegin = src.begin() + srcIndex;
				const auto srcEnd = srcBegin + RMDFile::DEPTH;
				const auto dstBegin = dst.get() + dstIndex;
				DebugAssert((dstBegin + std::distance(srcBegin, srcEnd)) <= dst.end());
				std::copy(srcBegin, srcEnd, dstBegin);
			};

			writeRow(rmd.getFLOR(), tempFlor);
			writeRow(rmd.getMAP1(), tempMap1);
			writeRow(rmd.getMAP2(), tempMap2);
		}
	};

	// Load .RMD files into the wilderness, each at some X and Z offset in the voxel grid.
	for (int y = 0; y < wildIndices.getHeight(); y++)
	{
		for (int x = 0; x < wildIndices.getWidth(); x++)
		{
			const uint8_t wildIndex = wildIndices.get(x, y);
			writeRMD(wildIndex, x * RMDFile::WIDTH, y * RMDFile::DEPTH);
		}
	}

	// Change the placeholder WILD00{1..4}.MIF blocks to the ones for the given city.
	ExteriorLevelData::reviseWildernessCity(
		localCityID, provinceID, tempFlor, tempMap1, tempMap2, miscAssets);

	// Create the level for the voxel data to be written into.
	const int levelHeight = 6;
	const std::string levelName = "WILD"; // Arbitrary
	ExteriorLevelData levelData(tempFlor.getWidth(), levelHeight, tempFlor.getHeight(),
		infName, levelName);

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	const auto &exeData = miscAssets.getExeData();
	const INFFile &inf = levelData.getInfFile();
	levelData.readFLOR(tempFlor.get(), inf, tempFlor.getWidth(), tempFlor.getHeight());
	levelData.readMAP1(tempMap1.get(), inf, WorldType::Wilderness, tempMap1.getWidth(),
		tempMap1.getHeight(), exeData);
	levelData.readMAP2(tempMap2.get(), inf, tempMap1.getWidth(), tempMap1.getHeight());
	// @todo: load FLAT from WILD.MIF level data. levelData.readFLAT(level.flat, ...)?

	// Generate wilderness building names.
	levelData.generateWildChunkBuildingNames(localCityID, provinceID, miscAssets);

	// Generate distant sky.
	levelData.distantSky.init(localCityID, provinceID, weatherType, currentDay,
		starCount, miscAssets, textureManager);

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

void ExteriorLevelData::setActive(TextureManager &textureManager, Renderer &renderer)
{
	LevelData::setActive(textureManager, renderer);

	// Give distant sky data to the renderer.
	renderer.setDistantSky(this->distantSky);
}

void ExteriorLevelData::tick(double dt)
{
	this->distantSky.tick(dt);
}
