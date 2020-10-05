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

namespace
{
	// Max height of .MIF/.RMD with highest MAP2 extension.
	constexpr int EXTERIOR_LEVEL_HEIGHT = 6;
}

ExteriorLevelData::ExteriorLevelData(SNInt gridWidth, int gridHeight, WEInt gridDepth,
	const std::string &infName, const std::string &name)
	: LevelData(gridWidth, gridHeight, gridDepth, infName, name) { }

ExteriorLevelData::~ExteriorLevelData()
{

}

ExteriorLevelData ExteriorLevelData::loadCity(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, const MIFFile::Level &level, WeatherType weatherType,
	int currentDay, int starCount, const std::string &infName, SNInt gridWidth, WEInt gridDepth,
	const MiscAssets &miscAssets, TextureManager &textureManager)
{
	// Create temp voxel data buffers and write the city skeleton data to them. Each city
	// block will be written to them as well.
	Buffer2D<uint16_t> tempFlor(gridDepth, gridWidth);
	Buffer2D<uint16_t> tempMap1(gridDepth, gridWidth);
	Buffer2D<uint16_t> tempMap2(gridDepth, gridWidth);
	CityLevelUtils::writeSkeleton(level,
		BufferView2D(tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight()),
		BufferView2D(tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight()),
		BufferView2D(tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight()));

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
	ExteriorLevelData levelData(gridWidth, EXTERIOR_LEVEL_HEIGHT, gridDepth, infName, level.getName());

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
	levelData.menuNames = CityLevelUtils::generateBuildingNames(locationDef, provinceDef, random,
		isCity, levelData.getVoxelGrid(), miscAssets);

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
	const std::string levelName = "WILD"; // Arbitrary
	ExteriorLevelData levelData(tempFlor.getWidth(), EXTERIOR_LEVEL_HEIGHT, tempFlor.getHeight(),
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
	levelData.menuNames = WildLevelUtils::generateWildChunkBuildingNames(
		levelData.getVoxelGrid(), exeData);

	// Generate distant sky.
	levelData.distantSky.init(locationDef, provinceDef, weatherType, currentDay,
		starCount, exeData, textureManager);

	return levelData;
}

const LevelUtils::MenuNamesList &ExteriorLevelData::getMenuNames() const
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
