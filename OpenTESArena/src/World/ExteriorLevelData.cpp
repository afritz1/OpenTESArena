#include <algorithm>
#include <iomanip>

#include "ExteriorLevelData.h"
#include "WorldType.h"
#include "../Assets/COLFile.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Rendering/Renderer.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/VoxelDataType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"

ExteriorLevelData::ExteriorLevelData(SNInt gridWidth, int gridHeight, WEInt gridDepth,
	const std::string &infName, const std::string &name)
	: LevelData(gridWidth, gridHeight, gridDepth, infName, name) { }

ExteriorLevelData::~ExteriorLevelData()
{

}

void ExteriorLevelData::generateCity(uint32_t citySeed, int cityDim, WEInt gridDepth,
	const std::vector<uint8_t> &reservedBlocks, const OriginalInt2 &startPosition,
	ArenaRandom &random, const MiscAssets &miscAssets, Buffer2D<uint16_t> &dstFlor,
	Buffer2D<uint16_t> &dstMap1, Buffer2D<uint16_t> &dstMap2)
{
	// Decide which city blocks to load.
	enum class BlockType
	{
		Empty, Reserved, Equipment, MagesGuild,
		NobleHouse, Temple, Tavern, Spacer, Houses
	};

	// Get the city's local X and Y, to be used later for building name generation.
	const Int2 localCityPoint = LocationUtils::getLocalCityPoint(citySeed);

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
	WEInt xDim = 0;
	SNInt zDim = 0;

	for (const BlockType block : plan)
	{
		if (block != BlockType::Reserved)
		{
			const int blockIndex = static_cast<int>(block) - 2;
			const std::string &blockCode = MIFUtils::getCityBlockCode(blockIndex);
			const std::string &rotation = MIFUtils::getCityBlockRotation(
				random.next() % MIFUtils::getCityBlockRotationCount());
			const int variationCount = MIFUtils::getCityBlockVariations(blockIndex);
			const int variation = std::max(random.next() % variationCount, 1);
			const std::string blockMifName = MIFUtils::makeCityBlockMifName(
				blockCode.c_str(), variation, rotation.c_str());

			// Load the block's .MIF data into the level.
			const auto &cityBlockMifs = miscAssets.getCityBlockMifs();
			const auto iter = cityBlockMifs.find(blockMifName);
			if (iter == cityBlockMifs.end())
			{
				DebugCrash("Could not find .MIF file \"" + blockMifName + "\".");
			}

			const MIFFile &blockMif = iter->second;
			const WEInt blockWidth = blockMif.getWidth();
			const SNInt blockDepth = blockMif.getDepth();
			const auto &blockLevel = blockMif.getLevels().front();
			const BufferView2D<const MIFFile::VoxelID> blockFLOR = blockLevel.getFLOR();
			const BufferView2D<const MIFFile::VoxelID> blockMAP1 = blockLevel.getMAP1();
			const BufferView2D<const MIFFile::VoxelID> blockMAP2 = blockLevel.getMAP2();

			// Offset of the block in the voxel grid.
			const WEInt xOffset = startPosition.x + (xDim * 20);
			const SNInt zOffset = startPosition.y + (zDim * 20);

			// Copy block data to temp buffers.
			for (SNInt z = 0; z < blockDepth; z++)
			{
				for (WEInt x = 0; x < blockWidth; x++)
				{
					const MIFFile::VoxelID srcFlorVoxel = blockFLOR.get(x, z);
					const MIFFile::VoxelID srcMap1Voxel = blockMAP1.get(x, z);
					const MIFFile::VoxelID srcMap2Voxel = blockMAP2.get(x, z);
					const WEInt dstX = xOffset + x;
					const SNInt dstZ = zOffset + z;
					dstFlor.set(dstX, dstZ, srcFlorVoxel);
					dstMap1.set(dstX, dstZ, srcMap1Voxel);
					dstMap2.set(dstX, dstZ, srcMap2Voxel);
				}
			}
		}

		xDim++;

		// Move to the next row if done with the current one.
		if (xDim == cityDim)
		{
			xDim = 0;
			zDim++;
		}
	}
}

void ExteriorLevelData::generateBuildingNames(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, ArenaRandom &random, bool isCity,
	SNInt gridWidth, WEInt gridDepth, const MiscAssets &miscAssets)
{
	const auto &exeData = miscAssets.getExeData();
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	
	uint32_t citySeed = cityDef.citySeed;
	const Int2 localCityPoint = LocationUtils::getLocalCityPoint(citySeed);

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [this, &provinceDef, &citySeed, &random, isCity,
		gridWidth, gridDepth, &miscAssets, &exeData, &cityDef, &localCityPoint](
			VoxelDefinition::WallData::MenuType menuType)
	{
		if ((menuType == VoxelDefinition::WallData::MenuType::Equipment) ||
			(menuType == VoxelDefinition::WallData::MenuType::Temple))
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
		auto createTavernName = [&exeData, &cityDef](int m, int n)
		{
			const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
			const auto &tavernSuffixes = cityDef.coastal ?
				exeData.cityGen.tavernMarineSuffixes : exeData.cityGen.tavernSuffixes;
			return tavernPrefixes.at(m) + ' ' + tavernSuffixes.at(n);
		};

		auto createEquipmentName = [&provinceDef, &random, gridWidth, gridDepth, &miscAssets,
			&exeData, &cityDef](int m, int n, SNInt x, WEInt z)
		{
			const auto &equipmentPrefixes = exeData.cityGen.equipmentPrefixes;
			const auto &equipmentSuffixes = exeData.cityGen.equipmentSuffixes;

			// Equipment store names can have variables in them.
			std::string str = equipmentPrefixes.at(m) + ' ' + equipmentSuffixes.at(n);

			// Replace %ct with city type name.
			size_t index = str.find("%ct");
			if (index != std::string::npos)
			{
				const std::string_view cityTypeName = cityDef.typeDisplayName;
				str.replace(index, 3, cityTypeName);
			}

			// Replace %ef with generated male first name from (y<<16)+x seed. Use a local RNG for
			// modifications to building names. Swap and reverse the XZ dimensions so they fit the
			// original XY values in Arena.
			index = str.find("%ef");
			if (index != std::string::npos)
			{
				ArenaRandom nameRandom((x << 16) + z);
				const bool isMale = true;
				const std::string maleFirstName = [&provinceDef, &miscAssets, isMale, &nameRandom]()
				{
					const std::string name = miscAssets.generateNpcName(
						provinceDef.getRaceID(), isMale, nameRandom);
					const std::string firstName = String::split(name).front();
					return firstName;
				}();

				str.replace(index, 3, maleFirstName);
			}

			// Replace %n with generated male name from (x<<16)+y seed.
			index = str.find("%n");
			if (index != std::string::npos)
			{
				ArenaRandom nameRandom((z << 16) + x);
				const bool isMale = true;
				const std::string maleName = miscAssets.generateNpcName(
					provinceDef.getRaceID(), isMale, nameRandom);
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
		auto tryGenerateBlockName = [this, isCity, menuType, &random, &seen, &hashInSeen,
			&createTavernName, &createEquipmentName, &createTempleName](SNInt x, WEInt z)
		{
			// See if the current voxel is a *MENU block and matches the target menu type.
			const bool matchesTargetType = [this, isCity, x, z, menuType]()
			{
				const auto &voxelGrid = this->getVoxelGrid();
				const uint16_t voxelID = voxelGrid.getVoxel(x, 1, z);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				return (voxelDef.dataType == VoxelDataType::Wall) && voxelDef.wall.isMenu() &&
					(VoxelDefinition::WallData::getMenuType(voxelDef.wall.menuID, isCity) == menuType);
			}();

			if (matchesTargetType)
			{
				// Get the *MENU block's display name.
				int hash;
				std::string name;

				if (menuType == VoxelDefinition::WallData::MenuType::Tavern)
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
				else if (menuType == VoxelDefinition::WallData::MenuType::Equipment)
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

				this->menuNames.push_back(std::make_pair(NewInt2(x, z), std::move(name)));
				seen.push_back(hash);
			}
		};

		// Start at the top-right corner of the map, running right to left and top to bottom.
		for (SNInt x = 0; x < gridWidth; x++)
		{
			for (WEInt z = 0; z < gridDepth; z++)
			{
				tryGenerateBlockName(x, z);
			}
		}

		// Fix some edge cases used with the main quest.
		if ((menuType == VoxelDefinition::WallData::MenuType::Temple) &&
			cityDef.hasMainQuestTempleOverride)
		{
			const auto &mainQuestTempleOverride = cityDef.mainQuestTempleOverride;
			const int modelIndex = mainQuestTempleOverride.modelIndex;
			const int suffixIndex = mainQuestTempleOverride.suffixIndex;

			// Added an index variable since the original game seems to store its menu names in a
			// way other than with a vector like this solution is using.
			const int menuNamesIndex = mainQuestTempleOverride.menuNamesIndex;

			DebugAssertIndex(this->menuNames, menuNamesIndex);
			this->menuNames[menuNamesIndex].second = createTempleName(modelIndex, suffixIndex);
		}
	};

	generateNames(VoxelDefinition::WallData::MenuType::Tavern);
	generateNames(VoxelDefinition::WallData::MenuType::Equipment);
	generateNames(VoxelDefinition::WallData::MenuType::Temple);
}

void ExteriorLevelData::generateWildChunkBuildingNames(const ExeData &exeData)
{
	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [this, &exeData](int wildX, int wildY,
		VoxelDefinition::WallData::MenuType menuType)
	{
		const uint32_t wildChunkSeed = (wildY << 16) + wildX;

		// Don't need hashInSeen() for the wilderness.

		// Lambdas for creating tavern and temple building names.
		auto createTavernName = [&exeData](int m, int n)
		{
			const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
			const auto &tavernSuffixes = exeData.cityGen.tavernSuffixes;
			return tavernPrefixes.at(m) + ' ' + tavernSuffixes.at(n);
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
		auto tryGenerateBlockName = [this, wildX, wildY, wildChunkSeed, menuType, &createTavernName,
			&createTempleName](SNInt x, WEInt z)
		{
			ArenaRandom random(wildChunkSeed);

			// Make sure the coordinate math is done in the new coordinate system.
			const OriginalInt2 relativeOrigin(
				((RMDFile::DEPTH - 1) - wildX) * RMDFile::DEPTH,
				((RMDFile::WIDTH - 1) - wildY) * RMDFile::WIDTH);
			const NewInt2 dstPoint(
				relativeOrigin.y + (RMDFile::WIDTH - 1 - x),
				relativeOrigin.x + (RMDFile::DEPTH - 1 - z));

			// See if the current voxel is a *MENU block and matches the target menu type.
			const bool matchesTargetType = [this, &dstPoint, menuType]()
			{
				const auto &voxelGrid = this->getVoxelGrid();
				const bool isCity = false; // Wilderness only.
				const uint16_t voxelID = voxelGrid.getVoxel(dstPoint.x, 1, dstPoint.y);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				return (voxelDef.dataType == VoxelDataType::Wall) && voxelDef.wall.isMenu() &&
					(VoxelDefinition::WallData::getMenuType(voxelDef.wall.menuID, isCity) == menuType);
			}();

			if (matchesTargetType)
			{
				// Get the *MENU block's display name.
				const std::string name = [menuType, &random, &createTavernName, &createTempleName]()
				{
					if (menuType == VoxelDefinition::WallData::MenuType::Tavern)
					{
						// Tavern.
						const int m = random.next() % 23;
						const int n = random.next() % 23;
						return createTavernName(m, n);
					}
					else
					{
						// Temple.
						const int model = random.next() % 3;
						constexpr std::array<int, 3> ModelVars = { 5, 9, 10 };
						const int vars = ModelVars.at(model);
						const int n = random.next() % vars;
						return createTempleName(model, n);
					}
				}();

				this->menuNames.push_back(std::make_pair(dstPoint, std::move(name)));
			}
		};

		// Iterate blocks in the chunk in any order. They are order-independent in the wild.
		for (SNInt x = 0; x < RMDFile::DEPTH; x++)
		{
			for (WEInt z = 0; z < RMDFile::WIDTH; z++)
			{
				tryGenerateBlockName(x, z);
			}
		}
	};

	// Iterate over each wild chunk.
	const int wildChunksPerSide = 64;
	for (int y = 0; y < wildChunksPerSide; y++)
	{
		for (int x = 0; x < wildChunksPerSide; x++)
		{
			generateNames(x, y, VoxelDefinition::WallData::MenuType::Tavern);
			generateNames(x, y, VoxelDefinition::WallData::MenuType::Temple);
		}
	}
}

void ExteriorLevelData::revisePalaceGraphics(Buffer2D<uint16_t> &map1, SNInt gridWidth, WEInt gridDepth)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [&map1, gridWidth, gridDepth](SNInt x, WEInt z)
	{
		const uint16_t voxel = map1.get(z, x);
		return voxel;
	};

	auto setMap1Voxel = [&map1, gridWidth, gridDepth](SNInt x, WEInt z, uint16_t voxel)
	{
		map1.set(z, x, voxel);
	};

	struct SearchResult
	{
		enum class Side { None, North, South, East, West };

		Side side;

		// Distance from the associated origin dimension, where (0, 0) is at the top right.
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
		auto isPalaceBlock = [&getMap1Voxel](SNInt x, WEInt z)
		{
			const uint16_t voxel = getMap1Voxel(x, z);
			const uint8_t mostSigNibble = (voxel & 0xF000) >> 12;
			return mostSigNibble == 0x9;
		};

		// North (top edge) and south (bottom edge), search right to left.
		for (WEInt z = 1; z < (gridDepth - 1); z++)
		{
			const SNInt northX = 0;
			const SNInt southX = gridWidth - 1;
			if (isPalaceBlock(northX, z))
			{
				return SearchResult(SearchResult::Side::North, z);
			}
			else if (isPalaceBlock(southX, z))
			{
				return SearchResult(SearchResult::Side::South, z);
			}
		}

		// East (right edge) and west (left edge), search top to bottom.
		for (SNInt x = 1; x < (gridWidth - 1); x++)
		{
			const WEInt eastZ = 0;
			const WEInt westZ = gridDepth - 1;
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
		const NewInt2 northSouthPalaceStep = VoxelUtils::West;
		const NewInt2 eastWestPalaceStep = VoxelUtils::South;

		// Gets the distance in voxels from a palace voxel to its gate, or -1 if no gate exists.
		const int NO_GATE = -1;
		auto getGateDistance = [&getMap1Voxel, NO_GATE](const NewInt2 &palaceVoxel, const NewInt2 &dir)
		{
			auto isGateBlock = [&getMap1Voxel](SNInt x, WEInt z)
			{
				const uint16_t voxel = getMap1Voxel(x, z);
				const uint8_t mostSigNibble = (voxel & 0xF000) >> 12;
				return mostSigNibble == 0xA;
			};

			// Gates should usually be within a couple blocks of their castle graphic. If not,
			// then no gate exists.
			const int MAX_GATE_DIST = 8;

			int i = 0;
			NewInt2 position = palaceVoxel;
			while ((i < MAX_GATE_DIST) && !isGateBlock(position.x, position.y))
			{
				position = position + dir;
				i++;
			}

			return (i < MAX_GATE_DIST) ? i : NO_GATE;
		};

		// Set the positions of the two palace voxels and the two gate voxels.
		NewInt2 firstPalaceVoxel, secondPalaceVoxel, firstGateVoxel, secondGateVoxel;
		uint16_t firstPalaceVoxelID, secondPalaceVoxelID, gateVoxelID;
		int gateDist;
		if (result.side == SearchResult::Side::North)
		{
			firstPalaceVoxel = NewInt2(0, result.offset);
			secondPalaceVoxel = firstPalaceVoxel + northSouthPalaceStep;
			const NewInt2 gateDir = VoxelUtils::South;
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + northSouthPalaceStep;
			firstPalaceVoxelID = 0xA5B4;
			secondPalaceVoxelID = 0xA5B5;
			gateVoxelID = 0xA1B3;
		}
		else if (result.side == SearchResult::Side::South)
		{
			firstPalaceVoxel = NewInt2(gridWidth - 1, result.offset);
			secondPalaceVoxel = firstPalaceVoxel + northSouthPalaceStep;
			const NewInt2 gateDir = VoxelUtils::North;
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + northSouthPalaceStep;
			firstPalaceVoxelID = 0xA535;
			secondPalaceVoxelID = 0xA534;
			gateVoxelID = 0xA133;
		}
		else if (result.side == SearchResult::Side::East)
		{
			firstPalaceVoxel = NewInt2(result.offset, 0);
			secondPalaceVoxel = firstPalaceVoxel + eastWestPalaceStep;
			const NewInt2 gateDir = VoxelUtils::West;
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + eastWestPalaceStep;
			firstPalaceVoxelID = 0xA575;
			secondPalaceVoxelID = 0xA574;
			gateVoxelID = 0xA173;
		}
		else if (result.side == SearchResult::Side::West)
		{
			firstPalaceVoxel = NewInt2(result.offset, gridDepth - 1);
			secondPalaceVoxel = firstPalaceVoxel + eastWestPalaceStep;
			const NewInt2 gateDir = VoxelUtils::East;
			gateDist = getGateDistance(firstPalaceVoxel, gateDir);
			firstGateVoxel = firstPalaceVoxel + (gateDir * gateDist);
			secondGateVoxel = firstGateVoxel + eastWestPalaceStep;
			firstPalaceVoxelID = 0xA5F4;
			secondPalaceVoxelID = 0xA5F5;
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

Buffer2D<ExteriorLevelData::WildBlockID> ExteriorLevelData::generateWildernessIndices(
	uint32_t wildSeed, const ExeData::Wilderness &wildData)
{
	constexpr int wildWidth = 64;
	constexpr int wildHeight = 64;
	Buffer2D<WildBlockID> indices(wildWidth, wildHeight);
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

void ExteriorLevelData::reviseWildernessCity(const LocationDefinition &locationDef,
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
	const WEInt xOffset = RMDFile::WIDTH * 31;
	const SNInt zOffset = RMDFile::DEPTH * 31;

	for (WEInt x = 0; x < placeholderWidth; x++)
	{
		const int startIndex = zOffset + ((x + xOffset) * flor.getHeight());

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

	// Get city generation info -- the .MIF filename to load for the city skeleton.
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const std::string mifName = cityDef.mapFilename;
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return;
	}

	const MIFFile::Level &level = mif.getLevels().front();
	const BufferView2D<const MIFFile::VoxelID> levelFLOR = level.getFLOR();
	const BufferView2D<const MIFFile::VoxelID> levelMAP1 = level.getMAP1();
	const BufferView2D<const MIFFile::VoxelID> levelMAP2 = level.getMAP2();

	// Buffers for the city data. Copy the .MIF data into them.
	Buffer2D<uint16_t> cityFlor(mif.getWidth(), mif.getDepth());
	Buffer2D<uint16_t> cityMap1(mif.getWidth(), mif.getDepth());
	Buffer2D<uint16_t> cityMap2(mif.getWidth(), mif.getDepth());

	for (WEInt x = 0; x < mif.getWidth(); x++)
	{
		for (SNInt z = 0; z < mif.getDepth(); z++)
		{
			const MIFFile::VoxelID srcFlorVoxel = levelFLOR.get(x, z);
			const MIFFile::VoxelID srcMap1Voxel = levelMAP1.get(x, z);
			const MIFFile::VoxelID srcMap2Voxel = levelMAP2.get(x, z);
			cityFlor.set(x, z, srcFlorVoxel);
			cityMap1.set(x, z, srcMap1Voxel);
			cityMap2.set(x, z, srcMap2Voxel);
		}
	}

	// Run city generation if it's not a premade city. The center province's city does not have
	// any special generation -- the .MIF buffers are simply used as-is (with some simple palace
	// gate revisions done afterwards).
	const bool isPremadeCity = cityDef.premade;
	if (!isPremadeCity)
	{
		const int cityBlocksPerSide = cityDef.cityBlocksPerSide;
		const std::vector<uint8_t> &reservedBlocks = *cityDef.reservedBlocks;
		const OriginalInt2 blockStartPosition(cityDef.blockStartPosX, cityDef.blockStartPosY);
		const uint32_t citySeed = cityDef.citySeed;
		ArenaRandom random(citySeed);

		// Write generated city data into the temp city buffers.
		ExteriorLevelData::generateCity(citySeed, cityBlocksPerSide, mif.getWidth(),
			reservedBlocks, blockStartPosition, random, miscAssets, cityFlor, cityMap1, cityMap2);
	}

	// Transform city voxels based on the wilderness rules.
	for (WEInt x = 0; x < mif.getWidth(); x++)
	{
		for (SNInt z = 0; z < mif.getDepth(); z++)
		{
			uint16_t &map1Voxel = cityMap1.get(x, z);
			uint16_t &map2Voxel = cityMap2.get(x, z);

			if ((map1Voxel & 0x8000) != 0)
			{
				map1Voxel = 0;
				map2Voxel = 0;
			}
			else
			{
				const bool isWall = (map1Voxel == 0x2F2F) || (map1Voxel == 0x2D2D) || (map1Voxel == 0x2E2E);
				if (!isWall)
				{
					map1Voxel = 0;
					map2Voxel = 0;
				}
				else
				{
					// Replace solid walls.
					if (map1Voxel == 0x2F2F)
					{
						map1Voxel = 0x3030;
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
					else if (map1Voxel == 0x2D2D)
					{
						map1Voxel = 0x2F2F;
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
					else if (map1Voxel == 0x2E2E)
					{
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
				}
			}
		}
	}

	// Write city buffers into the wilderness.
	for (SNInt z = 0; z < mif.getDepth(); z++)
	{
		for (WEInt x = 0; x < mif.getWidth(); x++)
		{
			const uint16_t srcFlorVoxel = cityFlor.get(x, z);
			const uint16_t srcMap1Voxel = cityMap1.get(x, z);
			const uint16_t srcMap2Voxel = cityMap2.get(x, z);
			const WEInt dstX = xOffset + x;
			const SNInt dstZ = zOffset + z;
			flor.set(dstX, dstZ, srcFlorVoxel);
			map1.set(dstX, dstZ, srcMap1Voxel);
			map2.set(dstX, dstZ, srcMap2Voxel);
		}
	}
}

OriginalInt2 ExteriorLevelData::getRelativeWildOrigin(const Int2 &voxel)
{
	return OriginalInt2(
		voxel.x - (voxel.x % (RMDFile::WIDTH * 2)),
		voxel.y - (voxel.y % (RMDFile::DEPTH * 2)));
}

NewInt2 ExteriorLevelData::getCenteredWildOrigin(const NewInt2 &voxel)
{
	return NewInt2(
		(std::max(voxel.x - 32, 0) / RMDFile::WIDTH) * RMDFile::WIDTH,
		(std::max(voxel.y - 32, 0) / RMDFile::DEPTH) * RMDFile::DEPTH);
}

ExteriorLevelData ExteriorLevelData::loadCity(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, const MIFFile::Level &level, WeatherType weatherType,
	int currentDay, int starCount, const std::string &infName, SNInt gridWidth, WEInt gridDepth,
	const MiscAssets &miscAssets, TextureManager &textureManager)
{
	const BufferView2D<const MIFFile::VoxelID> levelFLOR = level.getFLOR();
	const BufferView2D<const MIFFile::VoxelID> levelMAP1 = level.getMAP1();
	const BufferView2D<const MIFFile::VoxelID> levelMAP2 = level.getMAP2();
	const WEInt levelWidth = levelFLOR.getWidth();
	const SNInt levelDepth = levelFLOR.getHeight();

	// Create temp voxel data buffers and write the city skeleton data to them. Each city
	// block will be written to them as well.
	Buffer2D<uint16_t> tempFlor(levelWidth, levelDepth);
	Buffer2D<uint16_t> tempMap1(levelWidth, levelDepth);
	Buffer2D<uint16_t> tempMap2(levelWidth, levelDepth);

	for (WEInt x = 0; x < levelWidth; x++)
	{
		for (SNInt z = 0; z < levelDepth; z++)
		{
			const MIFFile::VoxelID srcFlorVoxel = levelFLOR.get(x, z);
			const MIFFile::VoxelID srcMap1Voxel = levelMAP1.get(x, z);
			const MIFFile::VoxelID srcMap2Voxel = levelMAP2.get(x, z);
			tempFlor.set(x, z, srcFlorVoxel);
			tempMap1.set(x, z, srcMap1Voxel);
			tempMap2.set(x, z, srcMap2Voxel);
		}
	}

	// Get the city's seed for random chunk generation. It is modified later during
	// building name generation.
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const uint32_t citySeed = cityDef.citySeed;
	ArenaRandom random(citySeed);

	if (!cityDef.premade)
	{
		// Generate procedural city data and write it into the temp buffers.
		const std::vector<uint8_t> &reservedBlocks = *cityDef.reservedBlocks;
		const OriginalInt2 blockStartPosition(cityDef.blockStartPosX, cityDef.blockStartPosY);
		ExteriorLevelData::generateCity(citySeed, cityDef.cityBlocksPerSide, gridDepth,
			reservedBlocks, blockStartPosition, random, miscAssets, tempFlor, tempMap1, tempMap2);
	}

	// Run the palace gate graphic algorithm over the perimeter of the MAP1 data.
	ExteriorLevelData::revisePalaceGraphics(tempMap1, gridWidth, gridDepth);

	// Create the level for the voxel data to be written into.
	ExteriorLevelData levelData(gridWidth, level.getHeight(), gridDepth, infName, level.getName());

	const BufferView2D<const MIFFile::VoxelID> tempFlorView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const MIFFile::VoxelID> tempMap1View(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());
	const BufferView2D<const MIFFile::VoxelID> tempMap2View(
		tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight());
	const INFFile &inf = levelData.getInfFile();
	const auto &exeData = miscAssets.getExeData();

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	levelData.readFLOR(tempFlorView, inf);
	levelData.readMAP1(tempMap1View, inf, WorldType::City, exeData);
	levelData.readMAP2(tempMap2View, inf);

	// Generate building names.
	const bool isCity = true;
	levelData.generateBuildingNames(locationDef, provinceDef, random, isCity,
		gridWidth, gridDepth, miscAssets);

	// Generate distant sky.
	levelData.distantSky.init(locationDef, provinceDef, weatherType, currentDay,
		starCount, exeData, textureManager);

	return levelData;
}

ExteriorLevelData ExteriorLevelData::loadWilderness(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, WeatherType weatherType, int currentDay,
	int starCount, const std::string &infName, const MiscAssets &miscAssets,
	TextureManager &textureManager)
{
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const auto &wildData = miscAssets.getExeData().wild;
	const Buffer2D<WildBlockID> wildIndices =
		ExteriorLevelData::generateWildernessIndices(cityDef.wildSeed, wildData);

	// Temp buffers for voxel data.
	Buffer2D<uint16_t> tempFlor(RMDFile::DEPTH * wildIndices.getWidth(),
		RMDFile::WIDTH * wildIndices.getHeight());
	Buffer2D<uint16_t> tempMap1(tempFlor.getWidth(), tempFlor.getHeight());
	Buffer2D<uint16_t> tempMap2(tempFlor.getWidth(), tempFlor.getHeight());
	tempFlor.fill(0);
	tempMap1.fill(0);
	tempMap2.fill(0);

	auto writeRMD = [&miscAssets, &tempFlor, &tempMap1, &tempMap2](
		uint8_t rmdID, WEInt xOffset, SNInt zOffset)
	{
		const std::vector<RMDFile> &rmdFiles = miscAssets.getWildernessChunks();
		const int rmdIndex = DebugMakeIndex(rmdFiles, rmdID - 1);
		const RMDFile &rmd = rmdFiles[rmdIndex];

		// Copy .RMD voxel data to temp buffers.
		const BufferView2D<const RMDFile::VoxelID> rmdFLOR = rmd.getFLOR();
		const BufferView2D<const RMDFile::VoxelID> rmdMAP1 = rmd.getMAP1();
		const BufferView2D<const RMDFile::VoxelID> rmdMAP2 = rmd.getMAP2();

		for (SNInt z = 0; z < RMDFile::DEPTH; z++)
		{
			for (WEInt x = 0; x < RMDFile::WIDTH; x++)
			{
				const RMDFile::VoxelID srcFlorVoxel = rmdFLOR.get(x, z);
				const RMDFile::VoxelID srcMap1Voxel = rmdMAP1.get(x, z);
				const RMDFile::VoxelID srcMap2Voxel = rmdMAP2.get(x, z);
				const WEInt dstX = xOffset + x;
				const SNInt dstZ = zOffset + z;
				tempFlor.set(dstX, dstZ, srcFlorVoxel);
				tempMap1.set(dstX, dstZ, srcMap1Voxel);
				tempMap2.set(dstX, dstZ, srcMap2Voxel);
			}
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
	ExteriorLevelData::reviseWildernessCity(locationDef, tempFlor, tempMap1, tempMap2, miscAssets);

	// Create the level for the voxel data to be written into.
	const int levelHeight = 6;
	const std::string levelName = "WILD"; // Arbitrary
	ExteriorLevelData levelData(tempFlor.getWidth(), levelHeight, tempFlor.getHeight(),
		infName, levelName);

	const BufferView2D<const MIFFile::VoxelID> tempFlorView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const MIFFile::VoxelID> tempMap1View(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());
	const BufferView2D<const MIFFile::VoxelID> tempMap2View(
		tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight());
	const INFFile &inf = levelData.getInfFile();
	const auto &exeData = miscAssets.getExeData();

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	levelData.readFLOR(tempFlorView, inf);
	levelData.readMAP1(tempMap1View, inf, WorldType::Wilderness, exeData);
	levelData.readMAP2(tempMap2View, inf);

	// Generate wilderness building names.
	levelData.generateWildChunkBuildingNames(exeData);

	// Generate distant sky.
	levelData.distantSky.init(locationDef, provinceDef, weatherType, currentDay,
		starCount, exeData, textureManager);

	return levelData;
}

const std::vector<std::pair<NewInt2, std::string>> &ExteriorLevelData::getMenuNames() const
{
	return this->menuNames;
}

bool ExteriorLevelData::isOutdoorDungeon() const
{
	return false;
}

void ExteriorLevelData::setActive(bool nightLightsAreActive, const WorldData &worldData,
	const LocationDefinition &locationDef, const CharacterClassLibrary &charClassLibrary,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	LevelData::setActive(nightLightsAreActive, worldData, locationDef, charClassLibrary,
		miscAssets, textureManager, renderer);

	// @todo: fetch this palette from somewhere better.
	COLFile col;
	const std::string colName = PaletteFile::fromName(PaletteName::Default);
	if (!col.init(colName.c_str()))
	{
		DebugCrash("Couldn't init .COL file \"" + colName + "\".");
	}

	// Give distant sky data to the renderer.
	renderer.setDistantSky(this->distantSky, col.getPalette(), textureManager);
}

void ExteriorLevelData::tick(Game &game, double dt)
{
	LevelData::tick(game, dt);
	this->distantSky.tick(dt);
}
