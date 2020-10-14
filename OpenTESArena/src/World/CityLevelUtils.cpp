#include "CityLevelUtils.h"
#include "LocationUtils.h"
#include "ProvinceDefinition.h"
#include "VoxelDataType.h"
#include "VoxelDefinition.h"
#include "VoxelGrid.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Math/Random.h"

#include "components/utilities/String.h"

void CityLevelUtils::writeSkeleton(const MIFFile::Level &level,
	BufferView2D<MIFFile::VoxelID> &dstFlor, BufferView2D<MIFFile::VoxelID> &dstMap1,
	BufferView2D<MIFFile::VoxelID> &dstMap2)
{
	const BufferView2D<const MIFFile::VoxelID> levelFLOR = level.getFLOR();
	const BufferView2D<const MIFFile::VoxelID> levelMAP1 = level.getMAP1();
	const BufferView2D<const MIFFile::VoxelID> levelMAP2 = level.getMAP2();
	const WEInt levelWidth = levelFLOR.getWidth();
	const SNInt levelDepth = levelFLOR.getHeight();

	for (WEInt x = 0; x < levelWidth; x++)
	{
		for (SNInt z = 0; z < levelDepth; z++)
		{
			const MIFFile::VoxelID srcFlorVoxel = levelFLOR.get(x, z);
			const MIFFile::VoxelID srcMap1Voxel = levelMAP1.get(x, z);
			const MIFFile::VoxelID srcMap2Voxel = levelMAP2.get(x, z);
			dstFlor.set(x, z, srcFlorVoxel);
			dstMap1.set(x, z, srcMap1Voxel);
			dstMap2.set(x, z, srcMap2Voxel);
		}
	}
}

void CityLevelUtils::generateCity(uint32_t citySeed, int cityDim, WEInt gridDepth,
	const std::vector<uint8_t> &reservedBlocks, const OriginalInt2 &startPosition,
	ArenaRandom &random, const BinaryAssetLibrary &binaryAssetLibrary, Buffer2D<uint16_t> &dstFlor,
	Buffer2D<uint16_t> &dstMap1, Buffer2D<uint16_t> &dstMap2)
{
	// Get the city's local X and Y, to be used later for building name generation.
	const Int2 localCityPoint = LocationUtils::getLocalCityPoint(citySeed);

	const int citySize = cityDim * cityDim;
	std::vector<MIFUtils::BlockType> plan(citySize, MIFUtils::BlockType::Empty);

	auto placeBlock = [citySize, &plan, &random](MIFUtils::BlockType blockType)
	{
		int planIndex;

		do
		{
			planIndex = random.next() % citySize;
		} while (plan.at(planIndex) != MIFUtils::BlockType::Empty);

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
			plan.at(block) = MIFUtils::BlockType::Reserved;
		}
	}

	// Initial block placement.
	placeBlock(MIFUtils::BlockType::Equipment);
	placeBlock(MIFUtils::BlockType::MagesGuild);
	placeBlock(MIFUtils::BlockType::NobleHouse);
	placeBlock(MIFUtils::BlockType::Temple);
	placeBlock(MIFUtils::BlockType::Tavern);
	placeBlock(MIFUtils::BlockType::Spacer);

	// Create city plan according to RNG.
	const int emptyBlocksInPlan = static_cast<int>(
		std::count(plan.begin(), plan.end(), MIFUtils::BlockType::Empty));
	for (int remainingBlocks = emptyBlocksInPlan; remainingBlocks > 0; remainingBlocks--)
	{
		const MIFUtils::BlockType blockType = MIFUtils::generateRandomBlockType(random);
		placeBlock(blockType);
	}

	// Build the city, loading data for each block. Load blocks right to left, top to bottom.
	WEInt xDim = 0;
	SNInt zDim = 0;

	for (const MIFUtils::BlockType block : plan)
	{
		if (block != MIFUtils::BlockType::Reserved)
		{
			const std::string blockMifName = MIFUtils::makeCityBlockMifName(block, random);

			// Load the block's .MIF data into the level.
			const auto &cityBlockMifs = binaryAssetLibrary.getCityBlockMifs();
			const auto iter = cityBlockMifs.find(blockMifName);
			if (iter == cityBlockMifs.end())
			{
				DebugCrash("Could not find .MIF file \"" + blockMifName + "\".");
			}

			const MIFFile &blockMif = iter->second;
			const WEInt blockWidth = blockMif.getWidth();
			const SNInt blockDepth = blockMif.getDepth();
			const auto &blockLevel = blockMif.getLevel(0);
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

LevelUtils::MenuNamesList CityLevelUtils::generateBuildingNames(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, ArenaRandom &random, bool isCity,
	const VoxelGrid &voxelGrid, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary)
{
	const auto &exeData = binaryAssetLibrary.getExeData();
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

	uint32_t citySeed = cityDef.citySeed;
	const Int2 localCityPoint = LocationUtils::getLocalCityPoint(citySeed);

	LevelUtils::MenuNamesList menuNames;

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [&provinceDef, &citySeed, &random, isCity, &voxelGrid, &textAssetLibrary,
		&exeData, &cityDef, &localCityPoint, &menuNames](VoxelDefinition::WallData::MenuType menuType)
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

		auto createEquipmentName = [&provinceDef, &random, &textAssetLibrary, &exeData,
			&cityDef](int m, int n, SNInt x, WEInt z)
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
				const std::string maleFirstName = [&provinceDef, &textAssetLibrary, isMale, &nameRandom]()
				{
					const std::string name = textAssetLibrary.generateNpcName(
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
				const std::string maleName = textAssetLibrary.generateNpcName(
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
		auto tryGenerateBlockName = [menuType, &random, isCity, &voxelGrid, &menuNames, &seen,
			&hashInSeen, &createTavernName, &createEquipmentName, &createTempleName](SNInt x, WEInt z)
		{
			// See if the current voxel is a *MENU block and matches the target menu type.
			const bool matchesTargetType = [x, z, menuType, isCity, &voxelGrid]()
			{
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

				menuNames.push_back(std::make_pair(NewInt2(x, z), std::move(name)));
				seen.push_back(hash);
			}
		};

		// Start at the top-right corner of the map, running right to left and top to bottom.
		for (SNInt x = 0; x < voxelGrid.getWidth(); x++)
		{
			for (WEInt z = 0; z < voxelGrid.getDepth(); z++)
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

			DebugAssertIndex(menuNames, menuNamesIndex);
			menuNames[menuNamesIndex].second = createTempleName(modelIndex, suffixIndex);
		}
	};

	generateNames(VoxelDefinition::WallData::MenuType::Tavern);
	generateNames(VoxelDefinition::WallData::MenuType::Equipment);
	generateNames(VoxelDefinition::WallData::MenuType::Temple);
	return menuNames;
}

void CityLevelUtils::revisePalaceGraphics(Buffer2D<uint16_t> &map1, SNInt gridWidth, WEInt gridDepth)
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
		constexpr int NO_GATE = -1;
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
			constexpr int MAX_GATE_DIST = 8;

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
