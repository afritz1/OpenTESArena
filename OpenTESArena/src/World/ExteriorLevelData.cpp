#include <algorithm>
#include <iomanip>

#include "CityLevelUtils.h"
#include "ExteriorLevelData.h"
#include "WildLevelUtils.h"
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
		CityLevelUtils::generateCity(citySeed, cityDef.cityBlocksPerSide, gridDepth,
			reservedBlocks, blockStartPosition, random, miscAssets, tempFlor, tempMap1, tempMap2);
	}

	// Run the palace gate graphic algorithm over the perimeter of the MAP1 data.
	CityLevelUtils::revisePalaceGraphics(tempMap1, gridWidth, gridDepth);

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
	const ExeData::Wilderness &wildData = miscAssets.getExeData().wild;
	const Buffer2D<WildBlockID> wildIndices =
		WildLevelUtils::generateWildernessIndices(cityDef.wildSeed, wildData);

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
	WildLevelUtils::reviseWildernessCity(locationDef, tempFlor, tempMap1, tempMap2, miscAssets);

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
	const ProvinceDefinition &provinceDef, const LocationDefinition &locationDef,
	const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
	const MiscAssets &miscAssets, Random &random, CitizenManager &citizenManager,
	TextureManager &textureManager, TextureInstanceManager &textureInstManager, Renderer &renderer)
{
	LevelData::setActive(nightLightsAreActive, worldData, provinceDef, locationDef, entityDefLibrary,
		charClassLibrary, miscAssets, random, citizenManager, textureManager, textureInstManager, renderer);

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
