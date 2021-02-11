#include <algorithm>
#include <functional>
#include <optional>

#include "ArenaCityUtils.h"
#include "ArenaInteriorUtils.h"
#include "ArenaLevelUtils.h"
#include "ArenaVoxelUtils.h"
#include "ArenaWildUtils.h"
#include "ChunkUtils.h"
#include "LevelData.h"
#include "LocationUtils.h"
#include "MapType.h"
#include "ProvinceDefinition.h"
#include "VoxelDefinition.h"
#include "VoxelFacing2D.h"
#include "WorldData.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CFAFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/DFAFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/IMGFile.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/RCIFile.h"
#include "../Assets/SETFile.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/CitizenManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/EntityType.h"
#include "../Entities/StaticEntity.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../Items/ArmorMaterialType.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

LevelData::FlatDef::FlatDef(ArenaTypes::FlatIndex flatIndex)
{
	this->flatIndex = flatIndex;
}

ArenaTypes::FlatIndex LevelData::FlatDef::getFlatIndex() const
{
	return this->flatIndex;
}

const std::vector<NewInt2> &LevelData::FlatDef::getPositions() const
{
	return this->positions;
}

void LevelData::FlatDef::addPosition(const NewInt2 &position)
{
	this->positions.push_back(position);
}

LevelData::Lock::Lock(const NewInt2 &position, int lockLevel)
	: position(position)
{
	this->lockLevel = lockLevel;
}

const NewInt2 &LevelData::Lock::getPosition() const
{
	return this->position;
}

int LevelData::Lock::getLockLevel() const
{
	return this->lockLevel;
}

LevelData::TextTrigger::TextTrigger(const std::string &text, bool displayedOnce)
	: text(text)
{
	this->displayedOnce = displayedOnce;
	this->previouslyDisplayed = false;
}

const std::string &LevelData::TextTrigger::getText() const
{
	return this->text;
}

bool LevelData::TextTrigger::isSingleDisplay() const
{
	return this->displayedOnce;
}

bool LevelData::TextTrigger::hasBeenDisplayed() const
{
	return this->previouslyDisplayed;
}

void LevelData::TextTrigger::setPreviouslyDisplayed(bool previouslyDisplayed)
{
	this->previouslyDisplayed = previouslyDisplayed;
}

void LevelData::Transition::init(const NewInt2 &voxel, Type type)
{
	this->voxel = voxel;
	this->type = type;
}

LevelData::Transition LevelData::Transition::makeLevelUp(const NewInt2 &voxel)
{
	Transition transition;
	transition.init(voxel, Transition::Type::LevelUp);
	return transition;
}

LevelData::Transition LevelData::Transition::makeLevelDown(const NewInt2 &voxel)
{
	Transition transition;
	transition.init(voxel, Transition::Type::LevelDown);
	return transition;
}

LevelData::Transition LevelData::Transition::makeMenu(const NewInt2 &voxel, int id)
{
	Transition transition;
	transition.init(voxel, Transition::Type::Menu);
	transition.menu.id = id;
	return transition;
}

const NewInt2 &LevelData::Transition::getVoxel() const
{
	return this->voxel;
}

LevelData::Transition::Type LevelData::Transition::getType() const
{
	return this->type;
}

const LevelData::Transition::Menu &LevelData::Transition::getMenu() const
{
	DebugAssert(this->type == Transition::Type::Menu);
	return this->menu;
}

void LevelData::Interior::init(uint32_t skyColor, bool outdoorDungeon)
{
	this->skyColor = skyColor;
	this->outdoorDungeon = outdoorDungeon;
}

LevelData::LevelData(SNInt gridWidth, int gridHeight, WEInt gridDepth, const std::string &infName,
	const std::string &name, bool isInterior)
	: voxelGrid(gridWidth, gridHeight, gridDepth), name(name)
{
	const int chunkCountX = (gridWidth + (RMDFile::WIDTH - 1)) / RMDFile::WIDTH;
	const int chunkCountY = (gridDepth + (RMDFile::DEPTH - 1)) / RMDFile::DEPTH;
	this->entityManager.init(chunkCountX, chunkCountY);

	if (!this->inf.init(infName.c_str()))
	{
		DebugCrash("Could not init .INF file \"" + infName + "\".");
	}

	this->isInterior = isInterior;
}

LevelData LevelData::loadInterior(const MIFFile::Level &level, SNInt gridWidth, WEInt gridDepth, const ExeData &exeData)
{
	// .INF filename associated with the interior level.
	const std::string infName = String::toUppercase(level.getInfo());

	constexpr bool isInterior = true;
	LevelData levelData(gridWidth, ArenaInteriorUtils::GRID_HEIGHT, gridDepth, infName, level.getName(), isInterior);

	const INFFile &inf = levelData.getInfFile();
	const bool outdoorDungeon = inf.getCeiling().outdoorDungeon;
	const uint32_t skyColor = (outdoorDungeon ? Color::Gray : Color::Black).toARGB(); // @todo: use color from palette
	levelData.interior.init(skyColor, outdoorDungeon);

	// Load FLOR and MAP1 voxels.
	constexpr MapType mapType = MapType::Interior;
	levelData.readFLOR(level.getFLOR(), inf, mapType);
	levelData.readMAP1(level.getMAP1(), inf, mapType, exeData);

	// All interiors have ceilings except some main quest dungeons which have a 1
	// as the third number after *CEILING in their .INF file.
	const bool hasCeiling = !inf.getCeiling().outdoorDungeon;

	// Fill the second floor with ceiling tiles if it's an "indoor dungeon". Otherwise,
	// leave it empty (for some "outdoor dungeons").
	if (hasCeiling)
	{
		levelData.readCeiling(inf);
	}

	// Assign locks.
	levelData.readLocks(level.getLOCK());

	// Assign text and sound triggers.
	levelData.readTriggers(level.getTRIG(), inf);

	return levelData;
}

LevelData LevelData::loadDungeon(ArenaRandom &random, const MIFFile &mif, int levelUpBlock,
	const int *levelDownBlock, int widthChunks, int depthChunks, const std::string &infName, SNInt gridWidth,
	WEInt gridDepth, const ExeData &exeData)
{
	// Create temp buffers for dungeon block data.
	Buffer2D<uint16_t> tempFlor(gridDepth, gridWidth);
	Buffer2D<uint16_t> tempMap1(gridDepth, gridWidth);
	tempFlor.fill(0);
	tempMap1.fill(0);

	std::vector<ArenaTypes::MIFLock> tempLocks;
	std::vector<ArenaTypes::MIFTrigger> tempTriggers;
	const int tileSet = random.next() % 4;

	for (SNInt row = 0; row < depthChunks; row++)
	{
		const SNInt zOffset = row * ArenaInteriorUtils::DUNGEON_CHUNK_DIM;
		for (WEInt column = 0; column < widthChunks; column++)
		{
			const WEInt xOffset = column * ArenaInteriorUtils::DUNGEON_CHUNK_DIM;

			// Get the selected level from the .MIF file.
			const int blockIndex = (tileSet * 8) + (random.next() % 8);
			const auto &blockLevel = mif.getLevel(blockIndex);
			const BufferView2D<const ArenaTypes::VoxelID> &blockFLOR = blockLevel.getFLOR();
			const BufferView2D<const ArenaTypes::VoxelID> &blockMAP1 = blockLevel.getMAP1();

			// Copy block data to temp buffers.
			for (SNInt z = 0; z < ArenaInteriorUtils::DUNGEON_CHUNK_DIM; z++)
			{
				for (WEInt x = 0; x < ArenaInteriorUtils::DUNGEON_CHUNK_DIM; x++)
				{
					const ArenaTypes::VoxelID srcFlorVoxel = blockFLOR.get(x, z);
					const ArenaTypes::VoxelID srcMap1Voxel = blockMAP1.get(x, z);
					const WEInt dstX = xOffset + x;
					const SNInt dstZ = zOffset + z;
					tempFlor.set(dstX, dstZ, srcFlorVoxel);
					tempMap1.set(dstX, dstZ, srcMap1Voxel);
				}
			}

			// Assign locks to the current block.
			const BufferView<const ArenaTypes::MIFLock> &blockLOCK = blockLevel.getLOCK();
			for (int i = 0; i < blockLOCK.getCount(); i++)
			{
				const auto &lock = blockLOCK.get(i);

				ArenaTypes::MIFLock tempLock;
				tempLock.x = xOffset + lock.x;
				tempLock.y = zOffset + lock.y;
				tempLock.lockLevel = lock.lockLevel;

				tempLocks.push_back(std::move(tempLock));
			}

			// Assign text/sound triggers to the current block.
			const BufferView<const ArenaTypes::MIFTrigger> &blockTRIG = blockLevel.getTRIG();
			for (int i = 0; i < blockTRIG.getCount(); i++)
			{
				const auto &trigger = blockTRIG.get(i);

				ArenaTypes::MIFTrigger tempTrigger;
				tempTrigger.x = xOffset + trigger.x;
				tempTrigger.y = zOffset + trigger.y;
				tempTrigger.textIndex = trigger.textIndex;
				tempTrigger.soundIndex = trigger.soundIndex;

				tempTriggers.push_back(std::move(tempTrigger));
			}
		}
	}

	// Dungeon (either named or in wilderness).
	constexpr bool isInterior = true;
	LevelData levelData(gridWidth, ArenaInteriorUtils::GRID_HEIGHT, gridDepth, infName, std::string(), isInterior);

	const uint32_t skyColor = Color::Black.toARGB(); // @todo: use color from palette
	constexpr bool outdoorDungeon = false;
	levelData.interior.init(skyColor, outdoorDungeon);

	// Draw perimeter blocks. First top and bottom, then right and left.
	constexpr ArenaTypes::VoxelID perimeterVoxel = 0x7800;
	for (WEInt x = 0; x < tempMap1.getWidth(); x++)
	{
		tempMap1.set(x, 0, perimeterVoxel);
		tempMap1.set(x, tempMap1.getHeight() - 1, perimeterVoxel);
	}

	for (SNInt z = 1; z < (tempMap1.getHeight() - 1); z++)
	{
		tempMap1.set(0, z, perimeterVoxel);
		tempMap1.set(tempMap1.getWidth() - 1, z, perimeterVoxel);
	}

	const INFFile &inf = levelData.getInfFile();

	// Put transition blocks, unless null.
	const uint8_t levelUpVoxelByte = *inf.getLevelUpIndex() + 1;
	WEInt levelUpX;
	SNInt levelUpZ;
	ArenaInteriorUtils::unpackLevelChangeVoxel(levelUpBlock, &levelUpX, &levelUpZ);
	tempMap1.set(ArenaInteriorUtils::offsetLevelChangeVoxel(levelUpX),
		ArenaInteriorUtils::offsetLevelChangeVoxel(levelUpZ),
		ArenaInteriorUtils::convertLevelChangeVoxel(levelUpVoxelByte));

	if (levelDownBlock != nullptr)
	{
		const uint8_t levelDownVoxelByte = *inf.getLevelDownIndex() + 1;
		WEInt levelDownX;
		SNInt levelDownZ;
		ArenaInteriorUtils::unpackLevelChangeVoxel(*levelDownBlock, &levelDownX, &levelDownZ);
		tempMap1.set(ArenaInteriorUtils::offsetLevelChangeVoxel(levelDownX),
			ArenaInteriorUtils::offsetLevelChangeVoxel(levelDownZ),
			ArenaInteriorUtils::convertLevelChangeVoxel(levelDownVoxelByte));
	}

	const BufferView2D<const ArenaTypes::VoxelID> tempFlorView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const ArenaTypes::VoxelID> tempMap1View(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());

	// Load FLOR, MAP1, and ceiling into the voxel grid.
	constexpr MapType mapType = MapType::Interior;
	levelData.readFLOR(tempFlorView, inf, mapType);
	levelData.readMAP1(tempMap1View, inf, mapType, exeData);
	levelData.readCeiling(inf);

	const BufferView<const ArenaTypes::MIFLock> tempLocksView(
		tempLocks.data(), static_cast<int>(tempLocks.size()));
	const BufferView<const ArenaTypes::MIFTrigger> tempTriggersView(
		tempTriggers.data(), static_cast<int>(tempTriggers.size()));

	// Load locks and triggers (if any).
	levelData.readLocks(tempLocksView);
	levelData.readTriggers(tempTriggersView, inf);

	return levelData;
}

LevelData LevelData::loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	const MIFFile::Level &level, WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
	SNInt gridWidth, WEInt gridDepth, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager)
{
	// Create temp voxel data buffers and write the city skeleton data to them. Each city
	// block will be written to them as well.
	Buffer2D<ArenaTypes::VoxelID> tempFlor(gridDepth, gridWidth);
	Buffer2D<ArenaTypes::VoxelID> tempMap1(gridDepth, gridWidth);
	Buffer2D<ArenaTypes::VoxelID> tempMap2(gridDepth, gridWidth);
	BufferView2D<ArenaTypes::VoxelID> tempFlorView(tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	BufferView2D<ArenaTypes::VoxelID> tempMap1View(tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());
	BufferView2D<ArenaTypes::VoxelID> tempMap2View(tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight());
	ArenaCityUtils::writeSkeleton(level, tempFlorView, tempMap1View, tempMap2View);

	// Get the city's seed for random chunk generation. It is modified later during
	// building name generation.
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const uint32_t citySeed = cityDef.citySeed;
	ArenaRandom random(citySeed);

	if (!cityDef.premade)
	{
		// Generate procedural city data and write it into the temp buffers.
		const BufferView<const uint8_t> reservedBlocks(cityDef.reservedBlocks->data(),
			static_cast<int>(cityDef.reservedBlocks->size()));
		const OriginalInt2 blockStartPosition(cityDef.blockStartPosX, cityDef.blockStartPosY);
		ArenaCityUtils::generateCity(citySeed, cityDef.cityBlocksPerSide, gridDepth, reservedBlocks,
			blockStartPosition, random, binaryAssetLibrary, tempFlor, tempMap1, tempMap2);
	}

	// Run the palace gate graphic algorithm over the perimeter of the MAP1 data.
	ArenaCityUtils::revisePalaceGraphics(tempMap1, gridWidth, gridDepth);

	// Create the level for the voxel data to be written into.
	constexpr bool isInterior = false;
	LevelData levelData(gridWidth, ArenaCityUtils::LEVEL_HEIGHT, gridDepth, infName, level.getName(), isInterior);

	const BufferView2D<const ArenaTypes::VoxelID> tempFlorConstView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const ArenaTypes::VoxelID> tempMap1ConstView(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());
	const BufferView2D<const ArenaTypes::VoxelID> tempMap2ConstView(
		tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight());
	const INFFile &inf = levelData.getInfFile();
	const auto &exeData = binaryAssetLibrary.getExeData();

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	constexpr MapType mapType = MapType::City;
	levelData.readFLOR(tempFlorConstView, inf, mapType);
	levelData.readMAP1(tempMap1ConstView, inf, mapType, exeData);
	levelData.readMAP2(tempMap2ConstView, inf);

	// Generate building names.
	levelData.exterior.menuNames = LevelData::generateBuildingNames(locationDef, provinceDef, random,
		levelData.getVoxelGrid(), levelData.getTransitions(), binaryAssetLibrary, textAssetLibrary);

	// Generate distant sky.
	levelData.exterior.distantSky.init(locationDef, provinceDef, weatherType, currentDay,
		starCount, exeData, textureManager);

	return levelData;
}

LevelData LevelData::loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager)
{
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const ExeData::Wilderness &wildData = binaryAssetLibrary.getExeData().wild;
	const Buffer2D<ArenaWildUtils::WildBlockID> wildIndices =
		ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, wildData);

	// Temp buffers for voxel data.
	Buffer2D<ArenaTypes::VoxelID> tempFlor(RMDFile::DEPTH * wildIndices.getWidth(),
		RMDFile::WIDTH * wildIndices.getHeight());
	Buffer2D<ArenaTypes::VoxelID> tempMap1(tempFlor.getWidth(), tempFlor.getHeight());
	Buffer2D<ArenaTypes::VoxelID> tempMap2(tempFlor.getWidth(), tempFlor.getHeight());
	tempFlor.fill(0);
	tempMap1.fill(0);
	tempMap2.fill(0);

	auto writeRMD = [&binaryAssetLibrary, &tempFlor, &tempMap1, &tempMap2](
		uint8_t rmdID, WEInt xOffset, SNInt zOffset)
	{
		const std::vector<RMDFile> &rmdFiles = binaryAssetLibrary.getWildernessChunks();
		const int rmdIndex = DebugMakeIndex(rmdFiles, rmdID - 1);
		const RMDFile &rmd = rmdFiles[rmdIndex];

		// Copy .RMD voxel data to temp buffers.
		const BufferView2D<const ArenaTypes::VoxelID> rmdFLOR = rmd.getFLOR();
		const BufferView2D<const ArenaTypes::VoxelID> rmdMAP1 = rmd.getMAP1();
		const BufferView2D<const ArenaTypes::VoxelID> rmdMAP2 = rmd.getMAP2();

		for (SNInt z = 0; z < RMDFile::DEPTH; z++)
		{
			for (WEInt x = 0; x < RMDFile::WIDTH; x++)
			{
				const ArenaTypes::VoxelID srcFlorVoxel = rmdFLOR.get(x, z);
				const ArenaTypes::VoxelID srcMap1Voxel = rmdMAP1.get(x, z);
				const ArenaTypes::VoxelID srcMap2Voxel = rmdMAP2.get(x, z);
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
	ArenaWildUtils::reviseWildernessCity(locationDef, tempFlor, tempMap1, tempMap2, binaryAssetLibrary);

	// Create the level for the voxel data to be written into.
	const std::string levelName = "WILD"; // Arbitrary
	constexpr bool isInterior = false;
	LevelData levelData(tempFlor.getWidth(), ArenaWildUtils::LEVEL_HEIGHT, tempFlor.getHeight(),
		infName, levelName, isInterior);

	const BufferView2D<const ArenaTypes::VoxelID> tempFlorView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const ArenaTypes::VoxelID> tempMap1View(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());
	const BufferView2D<const ArenaTypes::VoxelID> tempMap2View(
		tempMap2.get(), tempMap2.getWidth(), tempMap2.getHeight());
	const INFFile &inf = levelData.getInfFile();
	const auto &exeData = binaryAssetLibrary.getExeData();

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	constexpr MapType mapType = MapType::Wilderness;
	levelData.readFLOR(tempFlorView, inf, mapType);
	levelData.readMAP1(tempMap1View, inf, mapType, exeData);
	levelData.readMAP2(tempMap2View, inf);

	// Generate wilderness building names.
	levelData.exterior.menuNames = LevelData::generateWildChunkBuildingNames(
		levelData.getVoxelGrid(), levelData.getTransitions(), exeData);

	// Generate distant sky.
	levelData.exterior.distantSky.init(locationDef, provinceDef, weatherType, currentDay,
		starCount, exeData, textureManager);

	return levelData;
}

const std::string &LevelData::getName() const
{
	return this->name;
}

double LevelData::getCeilingHeight() const
{
	return static_cast<double>(this->inf.getCeiling().height) / MIFUtils::ARENA_UNITS;
}

std::vector<LevelData::FlatDef> &LevelData::getFlats()
{
	return this->flatsLists;
}

const std::vector<LevelData::FlatDef> &LevelData::getFlats() const
{
	return this->flatsLists;
}

LevelData::VoxelInstanceGroup *LevelData::tryGetVoxelInstances(const ChunkInt2 &chunk)
{
	const auto iter = this->voxelInstMap.find(chunk);
	if (iter != this->voxelInstMap.end())
	{
		return &iter->second;
	}
	else
	{
		return nullptr;
	}
}

const LevelData::VoxelInstanceGroup *LevelData::tryGetVoxelInstances(const ChunkInt2 &chunk) const
{
	const auto iter = this->voxelInstMap.find(chunk);
	if (iter != this->voxelInstMap.end())
	{
		return &iter->second;
	}
	else
	{
		return nullptr;
	}
}

VoxelInstance *LevelData::tryGetVoxelInstance(const NewInt3 &voxel, VoxelInstance::Type type)
{
	const ChunkInt2 chunk = VoxelUtils::newVoxelToChunk(NewInt2(voxel.x, voxel.z));
	VoxelInstanceGroup *voxelInstGroup = this->tryGetVoxelInstances(chunk);
	if (voxelInstGroup != nullptr)
	{
		const auto iter = voxelInstGroup->find(voxel);
		if (iter != voxelInstGroup->end())
		{
			std::vector<VoxelInstance> &voxelInsts = iter->second;
			std::optional<int> index;
			for (int i = 0; i < static_cast<int>(voxelInsts.size()); i++)
			{
				const VoxelInstance &voxelInst = voxelInsts[i];
				if (voxelInst.getType() == type)
				{
					index = i;
					break;
				}
			}

			if (index.has_value())
			{
				return &voxelInsts[*index];
			}
		}
	}

	return nullptr;
}

const VoxelInstance *LevelData::tryGetVoxelInstance(const NewInt3 &voxel, VoxelInstance::Type type) const
{
	const ChunkInt2 chunk = VoxelUtils::newVoxelToChunk(NewInt2(voxel.x, voxel.z));
	const VoxelInstanceGroup *voxelInstGroup = this->tryGetVoxelInstances(chunk);
	if (voxelInstGroup != nullptr)
	{
		const auto iter = voxelInstGroup->find(voxel);
		if (iter != voxelInstGroup->end())
		{
			const std::vector<VoxelInstance> &voxelInsts = iter->second;
			std::optional<int> index;
			for (int i = 0; i < static_cast<int>(voxelInsts.size()); i++)
			{
				const VoxelInstance &voxelInst = voxelInsts[i];
				if (voxelInst.getType() == type)
				{
					index = i;
					break;
				}
			}

			if (index.has_value())
			{
				return &voxelInsts[*index];
			}
		}
	}

	return nullptr;
}

LevelData::Transitions &LevelData::getTransitions()
{
	return this->transitions;
}

const LevelData::Transitions &LevelData::getTransitions() const
{
	return this->transitions;
}

const INFFile &LevelData::getInfFile() const
{
	return this->inf;
}

EntityManager &LevelData::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &LevelData::getEntityManager() const
{
	return this->entityManager;
}

VoxelGrid &LevelData::getVoxelGrid()
{
	return this->voxelGrid;
}

const VoxelGrid &LevelData::getVoxelGrid() const
{
	return this->voxelGrid;
}

const LevelData::Lock *LevelData::getLock(const NewInt2 &voxel) const
{
	const auto lockIter = this->locks.find(voxel);
	return (lockIter != this->locks.end()) ? &lockIter->second : nullptr;
}

LevelData::TextTrigger *LevelData::getTextTrigger(const NewInt2 &voxel)
{
	if (this->isInterior)
	{
		auto &textTriggers = this->interior.textTriggers;
		const auto textIter = textTriggers.find(voxel);
		return (textIter != textTriggers.end()) ? &textIter->second : nullptr;
	}
	else
	{
		return nullptr;
	}
}

const std::string *LevelData::getSoundTrigger(const NewInt2 &voxel) const
{
	if (this->isInterior)
	{
		const auto &soundTriggers = this->interior.soundTriggers;
		const auto soundIter = soundTriggers.find(voxel);
		return (soundIter != soundTriggers.end()) ? &soundIter->second : nullptr;
	}
	else
	{
		return nullptr;
	}
}

const ArenaLevelUtils::MenuNamesList &LevelData::getMenuNames() const
{
	DebugAssert(!this->isInterior);
	return this->exterior.menuNames;
}

bool LevelData::isOutdoorDungeon() const
{
	if (this->isInterior)
	{
		return this->interior.outdoorDungeon;
	}
	else
	{
		return false;
	}
}

void LevelData::addFlatInstance(ArenaTypes::FlatIndex flatIndex, const NewInt2 &flatPosition)
{
	// Add position to instance list if the flat def has already been created.
	const auto iter = std::find_if(this->flatsLists.begin(), this->flatsLists.end(),
		[flatIndex](const FlatDef &flatDef)
	{
		return flatDef.getFlatIndex() == flatIndex;
	});

	if (iter != this->flatsLists.end())
	{
		iter->addPosition(flatPosition);
	}
	else
	{
		// Create new def.
		FlatDef flatDef(flatIndex);
		flatDef.addPosition(flatPosition);
		this->flatsLists.push_back(std::move(flatDef));
	}
}

void LevelData::addVoxelInstance(VoxelInstance &&voxelInst)
{
	const ChunkInt2 chunk = VoxelUtils::newVoxelToChunk(NewInt2(voxelInst.getX(), voxelInst.getZ()));
	auto iter = this->voxelInstMap.find(chunk);
	if (iter == this->voxelInstMap.end())
	{
		iter = this->voxelInstMap.emplace(std::make_pair(chunk, VoxelInstanceGroup())).first;
	}

	VoxelInstanceGroup &voxelInstGroup = iter->second;
	const NewInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
	auto groupIter = voxelInstGroup.find(voxel);
	if (groupIter == voxelInstGroup.end())
	{
		groupIter = voxelInstGroup.emplace(std::make_pair(voxel, std::vector<VoxelInstance>())).first;
	}

	std::vector<VoxelInstance> &voxelInsts = groupIter->second;
	voxelInsts.emplace_back(std::move(voxelInst));
}

void LevelData::clearTemporaryVoxelInstances()
{
	for (auto &pair : this->voxelInstMap)
	{
		VoxelInstanceGroup &voxelInstGroup = pair.second;
		for (auto &groupPair : voxelInstGroup)
		{
			std::vector<VoxelInstance> &voxelInsts = groupPair.second;
			for (int i = static_cast<int>(voxelInsts.size()) - 1; i >= 0; i--)
			{
				VoxelInstance &voxelInst = voxelInsts[i];
				const VoxelInstance::Type voxelInstType = voxelInst.getType();
				const bool isTemporary = (voxelInstType == VoxelInstance::Type::OpenDoor) ||
					(voxelInstType == VoxelInstance::Type::Fading);

				if (isTemporary)
				{
					voxelInsts.erase(voxelInsts.begin() + i);
				}
			}
		}
	}
}

void LevelData::setVoxel(SNInt x, int y, WEInt z, uint16_t id)
{
	this->voxelGrid.setVoxel(x, y, z, id);
}

void LevelData::readFLOR(const BufferView2D<const ArenaTypes::VoxelID> &flor, const INFFile &inf,
	MapType mapType)
{
	const SNInt gridWidth = flor.getHeight();
	const WEInt gridDepth = flor.getWidth();

	// Lambda for obtaining a two-byte FLOR voxel.
	auto getFlorVoxel = [&flor, gridWidth, gridDepth](SNInt x, WEInt z)
	{
		const uint16_t voxel = flor.get(z, x);
		return voxel;
	};

	// Lambda for obtaining the voxel data index of a typical (non-chasm) FLOR voxel.
	auto getFlorDataIndex = [this, &inf, mapType](uint16_t florVoxel, int floorTextureID)
	{
		// See if the voxel already has a mapping.
		const auto floorIter = std::find_if(
			this->floorDataMappings.begin(), this->floorDataMappings.end(),
			[florVoxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == florVoxel;
		});

		if (floorIter != this->floorDataMappings.end())
		{
			return floorIter->second;
		}
		else
		{
			// Insert new mapping.
			const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(floorTextureID);
			TextureAssetReference textureAssetRef(
				ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
				ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
			const bool isWildWallColored = ArenaVoxelUtils::isFloorWildWallColored(floorTextureID, mapType);
			const int index = this->voxelGrid.addVoxelDef(
				VoxelDefinition::makeFloor(std::move(textureAssetRef), isWildWallColored));
			this->floorDataMappings.push_back(std::make_pair(florVoxel, index));
			return index;
		}
	};

	using ChasmDataFunc = VoxelDefinition(*)(const INFFile &inf);

	// Lambda for obtaining the voxel data index of a chasm voxel. The given function argument
	// returns the created voxel data if there was no previous mapping.
	auto getChasmDataIndex = [this, &inf](uint16_t florVoxel, ChasmDataFunc chasmFunc)
	{
		const auto floorIter = std::find_if(
			this->floorDataMappings.begin(), this->floorDataMappings.end(),
			[florVoxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == florVoxel;
		});

		if (floorIter != this->floorDataMappings.end())
		{
			return floorIter->second;
		}
		else
		{
			// Insert new mapping.
			const int index = this->voxelGrid.addVoxelDef(chasmFunc(inf));
			this->floorDataMappings.push_back(std::make_pair(florVoxel, index));
			return index;
		}
	};

	// Helper lambdas for creating each type of chasm voxel data.
	auto makeDryChasmVoxelDef = [](const INFFile &inf)
	{
		const int dryChasmID = [&inf]()
		{
			const std::optional<int> &index = inf.getDryChasmIndex();
			if (index.has_value())
			{
				return *index;
			}
			else
			{
				DebugLogWarning("Missing *DRYCHASM ID.");
				return 0;
			}
		}();

		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(dryChasmID);
		TextureAssetReference textureAssetRef(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
		return VoxelDefinition::makeChasm(std::move(textureAssetRef), ArenaTypes::ChasmType::Dry);
	};

	auto makeLavaChasmVoxelDef = [](const INFFile &inf)
	{
		const int lavaChasmID = [&inf]()
		{
			const std::optional<int> &index = inf.getLavaChasmIndex();
			if (index.has_value())
			{
				return *index;
			}
			else
			{
				DebugLogWarning("Missing *LAVACHASM ID.");
				return 0;
			}
		}();

		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(lavaChasmID);
		TextureAssetReference textureAssetRef(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
		return VoxelDefinition::makeChasm(std::move(textureAssetRef), ArenaTypes::ChasmType::Lava);
	};

	auto makeWetChasmVoxelDef = [](const INFFile &inf)
	{
		const int wetChasmID = [&inf]()
		{
			const std::optional<int> &index = inf.getWetChasmIndex();
			if (index.has_value())
			{
				return *index;
			}
			else
			{
				DebugLogWarning("Missing *WETCHASM ID.");
				return 0;
			}
		}();

		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(wetChasmID);
		TextureAssetReference textureAssetRef(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
		return VoxelDefinition::makeChasm(std::move(textureAssetRef), ArenaTypes::ChasmType::Wet);
	};

	// Write the voxel IDs into the voxel grid.
	for (SNInt x = 0; x < gridWidth; x++)
	{
		for (WEInt z = 0; z < gridDepth; z++)
		{
			auto getFloorTextureID = [](uint16_t voxel)
			{
				return (voxel & 0xFF00) >> 8;
			};

			auto getFloorFlatID = [](uint16_t voxel)
			{
				return voxel & 0x00FF;
			};

			const uint16_t florVoxel = getFlorVoxel(x, z);
			const int floorTextureID = getFloorTextureID(florVoxel);

			// See if the floor voxel is either solid or a chasm.
			if (!MIFUtils::isChasm(floorTextureID))
			{
				// Get the voxel data index associated with the floor value, or add it
				// if it doesn't exist yet.
				const int dataIndex = getFlorDataIndex(florVoxel, floorTextureID);
				this->setVoxel(x, 0, z, dataIndex);
			}
			else
			{
				// Chasm of some type.
				ChasmDataFunc chasmDataFunc;
				if (floorTextureID == MIFUtils::DRY_CHASM)
				{
					chasmDataFunc = makeDryChasmVoxelDef;
				}
				else if (floorTextureID == MIFUtils::LAVA_CHASM)
				{
					chasmDataFunc = makeLavaChasmVoxelDef;
				}
				else if (floorTextureID == MIFUtils::WET_CHASM)
				{
					chasmDataFunc = makeWetChasmVoxelDef;
				}
				else
				{
					DebugNotImplementedMsg(std::to_string(floorTextureID));
				}

				const int dataIndex = getChasmDataIndex(florVoxel, chasmDataFunc);
				this->setVoxel(x, 0, z, dataIndex);
			}

			// See if the FLOR voxel contains a FLAT index (for raised platform flats).
			const int floorFlatID = getFloorFlatID(florVoxel);
			if (floorFlatID > 0)
			{
				const ArenaTypes::FlatIndex flatIndex = floorFlatID - 1;
				this->addFlatInstance(flatIndex, NewInt2(x, z));
			}
		}
	}

	// Set chasm faces based on adjacent voxels.
	for (SNInt x = 0; x < gridWidth; x++)
	{
		for (WEInt z = 0; z < gridDepth; z++)
		{
			const NewInt3 voxel(x, 0, z);

			// Ignore non-chasm voxels.
			const uint16_t voxelID = this->voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
			const VoxelDefinition &voxelDef = this->voxelGrid.getVoxelDef(voxelID);
			if (voxelDef.type != ArenaTypes::VoxelType::Chasm)
			{
				continue;
			}

			// Query surrounding voxels to see which faces should be set.
			uint16_t northID, southID, eastID, westID;
			this->getAdjacentVoxelIDs(voxel, &northID, &southID, &eastID, &westID);

			const VoxelDefinition &northDef = this->voxelGrid.getVoxelDef(northID);
			const VoxelDefinition &southDef = this->voxelGrid.getVoxelDef(southID);
			const VoxelDefinition &eastDef = this->voxelGrid.getVoxelDef(eastID);
			const VoxelDefinition &westDef = this->voxelGrid.getVoxelDef(westID);

			// Booleans for each face of the new chasm voxel.
			const bool hasNorthFace = northDef.allowsChasmFace();
			const bool hasSouthFace = southDef.allowsChasmFace();
			const bool hasEastFace = eastDef.allowsChasmFace();
			const bool hasWestFace = westDef.allowsChasmFace();

			// Add chasm state if it is different from the default 0 faces chasm (don't need to
			// do update on existing chasms here because there should be no existing ones).
			const bool shouldAddChasmState = hasNorthFace || hasEastFace || hasSouthFace || hasWestFace;
			if (shouldAddChasmState)
			{
				VoxelInstance voxelInst = VoxelInstance::makeChasm(
					x, 0, z, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace);
				this->addVoxelInstance(std::move(voxelInst));
			}
		}
	}
}

void LevelData::readMAP1(const BufferView2D<const ArenaTypes::VoxelID> &map1, const INFFile &inf,
	MapType mapType, const ExeData &exeData)
{
	const SNInt gridWidth = map1.getHeight();
	const WEInt gridDepth = map1.getWidth();

	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [&map1, gridWidth, gridDepth](SNInt x, WEInt z)
	{
		const uint16_t voxel = map1.get(z, x);
		return voxel;
	};

	// Lambda for finding if there's an existing mapping for a MAP1 voxel.
	auto findWallMapping = [this](uint16_t map1Voxel)
	{
		return std::find_if(this->wallDataMappings.begin(), this->wallDataMappings.end(),
			[map1Voxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == map1Voxel;
		});
	};

	// Lambda for obtaining the voxel data index of a general-case MAP1 object. The function
	// parameter returns the created voxel data if no previous mapping exists, and is intended
	// for general cases where the voxel data type does not need extra parameters.
	auto getDataIndex = [this, &findWallMapping](uint16_t map1Voxel, VoxelDefinition(*func)(uint16_t))
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			const int index = this->voxelGrid.addVoxelDef(func(map1Voxel));
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a solid wall.
	auto getWallDataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel, uint8_t mostSigByte,
		SNInt x, WEInt z)
	{
		const int textureIndex = mostSigByte - 1;

		// Menu index if the voxel has the *MENU tag, or empty if it is not a *MENU voxel.
		const std::optional<int> &menuIndex = inf.getMenuIndex(textureIndex);
		const bool isMenu = menuIndex.has_value();

		// Lambda for whether an .INF file *LEVELUP/LEVELDOWN index is for this texture.
		auto isMatchingLevelChangeIndex = [textureIndex](const std::optional<int> &index)
		{
			return index.has_value() && (*index == textureIndex);
		};

		const bool isLevelUp = isMatchingLevelChangeIndex(inf.getLevelUpIndex());
		const bool isLevelDown = isMatchingLevelChangeIndex(inf.getLevelDownIndex());

		// Optionally add transition data for this voxel if it is a transition (level change or *MENU).
		if (isLevelUp || isLevelDown || isMenu)
		{
			auto makeWallTransition = [](const NewInt2 &voxel, const std::optional<bool> &levelUp,
				const std::optional<int> &menuID)
			{
				if (levelUp.has_value())
				{
					return *levelUp ? LevelData::Transition::makeLevelUp(voxel) :
						LevelData::Transition::makeLevelDown(voxel);
				}
				else
				{
					DebugAssert(menuID.has_value());
					return LevelData::Transition::makeMenu(voxel, *menuID);
				}
			};

			const NewInt2 voxel(x, z);
			const std::optional<bool> optIsLevelUp = [isLevelUp, isLevelDown, isMenu]() -> std::optional<bool>
			{
				if (isLevelUp)
				{
					return true;
				}
				else if (isLevelDown)
				{
					return false;
				}
				else
				{
					return std::nullopt;
				}
			}();

			this->transitions.emplace(voxel, makeWallTransition(voxel, optIsLevelUp, menuIndex));
		}

		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
			const TextureAssetReference textureAssetRef(
				ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
				ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
			const int index = this->voxelGrid.addVoxelDef(VoxelDefinition::makeWall(
				TextureAssetReference(textureAssetRef), TextureAssetReference(textureAssetRef),
				TextureAssetReference(textureAssetRef)));
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a raised platform.
	auto getRaisedDataIndex = [this, &inf, mapType, &exeData, &findWallMapping](
		uint16_t map1Voxel, uint8_t mostSigByte, SNInt x, WEInt z)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating a raised voxel data.
			auto makeRaisedVoxelData = [mapType, &exeData, map1Voxel, &inf, mostSigByte, x, z]()
			{
				const uint8_t wallTextureID = map1Voxel & 0x000F;
				const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

				const int sideID = [&inf, wallTextureID]()
				{
					const std::optional<int> &id = inf.getBoxSide(wallTextureID);
					if (id.has_value())
					{
						return *id;
					}
					else
					{
						DebugLogWarning("Missing *BOXSIDE ID \"" +
							std::to_string(wallTextureID) + "\".");
						return 0;
					}
				}();

				const int floorID = [&inf, x, z]()
				{
					const auto &id = inf.getCeiling().textureIndex;

					if (id.has_value())
					{
						return id.value();
					}
					else
					{
						DebugLogWarning("Missing platform floor ID (" +
							std::to_string(x) + ", " + std::to_string(z) + ").");
						return 0;
					}
				}();

				const int ceilingID = [&inf, capTextureID]()
				{
					const std::optional<int> &id = inf.getBoxCap(capTextureID);
					if (id.has_value())
					{
						return *id;
					}
					else
					{
						DebugLogWarning("Missing *BOXCAP ID \"" +
							std::to_string(capTextureID) + "\".");
						return 0;
					}
				}();

				const auto &wallHeightTables = exeData.wallHeightTables;
				const int heightIndex = mostSigByte & 0x07;
				const int thicknessIndex = (mostSigByte & 0x78) >> 3;
				int baseOffset, baseSize;
				
				if (mapType == MapType::Interior)
				{
					baseOffset = wallHeightTables.box1a.at(heightIndex);

					const int boxSize = wallHeightTables.box2a.at(thicknessIndex);
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = boxScale.has_value() ?
						((boxSize * (*boxScale)) / 256) : boxSize;
				}
				else if (mapType == MapType::City)
				{
					baseOffset = wallHeightTables.box1b.at(heightIndex);
					baseSize = wallHeightTables.box2b.at(thicknessIndex);
				}
				else if (mapType == MapType::Wilderness)
				{
					baseOffset = wallHeightTables.box1c.at(heightIndex);

					const int boxSize = 32;
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = (boxSize *
						(boxScale.has_value() ? boxScale.value() : 192)) / 256;
				}
				else
				{
					throw DebugException("Invalid world type \"" +
						std::to_string(static_cast<int>(mapType)) + "\".");
				}

				const double yOffset = static_cast<double>(baseOffset) / MIFUtils::ARENA_UNITS;
				const double ySize = static_cast<double>(baseSize) / MIFUtils::ARENA_UNITS;
				const double normalizedScale = static_cast<double>(inf.getCeiling().height) / MIFUtils::ARENA_UNITS;
				const double yOffsetNormalized = yOffset / normalizedScale;
				const double ySizeNormalized = ySize / normalizedScale;

				// @todo: might need some tweaking with box3/box4 values.
				const double vTop = std::max(
					0.0, 1.0 - yOffsetNormalized - ySizeNormalized);
				const double vBottom = std::min(vTop + ySizeNormalized, 1.0);

				const int clampedSideID = ArenaVoxelUtils::clampVoxelTextureID(sideID);
				const int clampedFloorID = ArenaVoxelUtils::clampVoxelTextureID(floorID);
				const int clampedCeilingID = ArenaVoxelUtils::clampVoxelTextureID(ceilingID);
				TextureAssetReference sideTextureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedSideID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedSideID, inf));
				TextureAssetReference floorTextureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedFloorID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedFloorID, inf));
				TextureAssetReference ceilingTextureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedCeilingID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedCeilingID, inf));
				return VoxelDefinition::makeRaised(std::move(sideTextureAssetRef), std::move(floorTextureAssetRef),
					std::move(ceilingTextureAssetRef), yOffsetNormalized, ySizeNormalized, vTop, vBottom);
			};

			const int index = this->voxelGrid.addVoxelDef(makeRaisedVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a type 0x9 voxel.
	auto getType9DataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0x9 voxel data.
			auto makeType9VoxelData = [&inf, map1Voxel]()
			{
				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				TextureAssetReference textureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				const bool collider = (map1Voxel & 0x0100) == 0;
				return VoxelDefinition::makeTransparentWall(std::move(textureAssetRef), collider);
			};

			const int index = this->voxelGrid.addVoxelDef(makeType9VoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a type 0xA voxel.
	auto getTypeADataIndex = [this, &inf, mapType, &findWallMapping](uint16_t map1Voxel, int textureIndex)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0xA voxel data.
			auto makeTypeAVoxelData = [&inf, mapType, map1Voxel, textureIndex]()
			{
				const double yOffset = [mapType, map1Voxel]()
				{
					const int baseOffset = (map1Voxel & 0x0E00) >> 9;
					const int fullOffset = (mapType == MapType::Interior) ?
						(baseOffset * 8) : ((baseOffset * 32) - 8);

					return static_cast<double>(fullOffset) / MIFUtils::ARENA_UNITS;
				}();

				const bool collider = (map1Voxel & 0x0100) != 0;

				// "Flipped" is not present in the original game, but has been added
				// here so that all edge voxel texture coordinates (i.e., palace
				// graphics, store signs) can be correct. Currently only palace
				// graphics and gates are type 0xA colliders, I believe.
				const bool flipped = collider;

				const VoxelFacing2D facing = [map1Voxel]()
				{
					// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
					// and C is east. It is stored in two bits above the texture index.
					const int orientation = (map1Voxel & 0x00C0) >> 4;
					if (orientation == 0x0)
					{
						return VoxelFacing2D::NegativeX;
					}
					else if (orientation == 0x4)
					{
						return VoxelFacing2D::PositiveZ;
					}
					else if (orientation == 0x8)
					{
						return VoxelFacing2D::PositiveX;
					}
					else
					{
						return VoxelFacing2D::NegativeZ;
					}
				}();

				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				TextureAssetReference textureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				return VoxelDefinition::makeEdge(std::move(textureAssetRef), yOffset, collider, flipped, facing);
			};

			const int index = this->voxelGrid.addVoxelDef(makeTypeAVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a type 0xB voxel.
	auto getTypeBDataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0xB voxel data.
			auto makeTypeBVoxelData = [&inf, map1Voxel]()
			{
				const int textureIndex = (map1Voxel & 0x003F) - 1;
				const ArenaTypes::DoorType doorType = [map1Voxel]()
				{
					const int type = (map1Voxel & 0x00C0) >> 4;
					if (type == 0x0)
					{
						return ArenaTypes::DoorType::Swinging;
					}
					else if (type == 0x4)
					{
						return ArenaTypes::DoorType::Sliding;
					}
					else if (type == 0x8)
					{
						return ArenaTypes::DoorType::Raising;
					}
					else
					{
						// I don't believe any doors in Arena split (but they are
						// supported by the engine).
						DebugUnhandledReturnMsg(ArenaTypes::DoorType, std::to_string(type));
					}
				}();

				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				TextureAssetReference textureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				return VoxelDefinition::makeDoor(std::move(textureAssetRef), doorType);
			};

			const int index = this->voxelGrid.addVoxelDef(makeTypeBVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a type 0xD voxel.
	auto getTypeDDataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0xD voxel data.
			auto makeTypeDVoxelData = [&inf, map1Voxel]()
			{
				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				TextureAssetReference textureAssetRef(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				const bool isRightDiag = (map1Voxel & 0x0100) == 0;
				return VoxelDefinition::makeDiagonal(std::move(textureAssetRef), isRightDiag);
			};

			const int index = this->voxelGrid.addVoxelDef(makeTypeDVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Write the voxel IDs into the voxel grid.
	for (SNInt x = 0; x < gridWidth; x++)
	{
		for (WEInt z = 0; z < gridDepth; z++)
		{
			const uint16_t map1Voxel = getMap1Voxel(x, z);

			if ((map1Voxel & 0x8000) == 0)
			{
				// A voxel of some kind.
				const bool voxelIsEmpty = map1Voxel == 0;

				if (!voxelIsEmpty)
				{
					const uint8_t mostSigByte = (map1Voxel & 0x7F00) >> 8;
					const uint8_t leastSigByte = map1Voxel & 0x007F;
					const bool voxelIsSolid = mostSigByte == leastSigByte;

					if (voxelIsSolid)
					{
						// Regular solid wall.
						const int dataIndex = getWallDataIndex(map1Voxel, mostSigByte, x, z);
						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = getRaisedDataIndex(map1Voxel, mostSigByte, x, z);
						this->setVoxel(x, 1, z, dataIndex);
					}
				}
			}
			else
			{
				// A special voxel, or an object of some kind.
				const uint8_t mostSigNibble = (map1Voxel & 0xF000) >> 12;

				if (mostSigNibble == 0x8)
				{
					// The lower byte determines the index of a FLAT for an object.
					const ArenaTypes::FlatIndex flatIndex = map1Voxel & 0x00FF;
					this->addFlatInstance(flatIndex, NewInt2(x, z));
				}
				else if (mostSigNibble == 0x9)
				{
					// Transparent block with 1-sided texture on all sides, such as wooden 
					// arches in dungeons. These do not have back-faces (especially when 
					// standing in the voxel itself).
					const int dataIndex = getType9DataIndex(map1Voxel);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int textureIndex = (map1Voxel & 0x003F) - 1;

					// It is clamped non-negative due to a case in the center province's city
					// where one temple voxel has all zeroes for its texture index, and it
					// appears solid gray in the original game (presumably a silent bug).
					if (textureIndex >= 0)
					{
						const int dataIndex = getTypeADataIndex(map1Voxel, textureIndex);
						this->setVoxel(x, 1, z, dataIndex);
					}
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = getTypeBDataIndex(map1Voxel);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown.
					DebugLogWarning("Voxel type 0xC not implemented.");
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall. Its type is determined by the nineth bit.
					const int dataIndex = getTypeDDataIndex(map1Voxel);
					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const BufferView2D<const ArenaTypes::VoxelID> &map2, const INFFile &inf)
{
	const SNInt gridWidth = map2.getHeight();
	const WEInt gridDepth = map2.getWidth();

	// Lambda for obtaining a two-byte MAP2 voxel.
	auto getMap2Voxel = [&map2, gridWidth, gridDepth](SNInt x, WEInt z)
	{
		const uint16_t voxel = map2.get(z, x);
		return voxel;
	};

	// Lambda for obtaining the voxel data index for a MAP2 voxel.
	auto getMap2DataIndex = [this, &inf](uint16_t map2Voxel)
	{
		const auto map2Iter = std::find_if(
			this->map2DataMappings.begin(), this->map2DataMappings.end(),
			[map2Voxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == map2Voxel;
		});

		if (map2Iter != this->map2DataMappings.end())
		{
			return map2Iter->second;
		}
		else
		{
			const int textureIndex = (map2Voxel & 0x007F) - 1;
			const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
			const TextureAssetReference textureAssetRef(
				ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
				ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
			const int index = this->voxelGrid.addVoxelDef(VoxelDefinition::makeWall(
				TextureAssetReference(textureAssetRef), TextureAssetReference(textureAssetRef),
				TextureAssetReference(textureAssetRef)));
			this->map2DataMappings.push_back(std::make_pair(map2Voxel, index));
			return index;
		}
	};

	// Write the voxel IDs into the voxel grid.
	for (SNInt x = 0; x < gridWidth; x++)
	{
		for (WEInt z = 0; z < gridDepth; z++)
		{
			const uint16_t map2Voxel = getMap2Voxel(x, z);

			if (map2Voxel != 0)
			{
				const int height = ArenaLevelUtils::getMap2VoxelHeight(map2Voxel);
				const int dataIndex = getMap2DataIndex(map2Voxel);

				for (int y = 2; y < (height + 2); y++)
				{
					this->setVoxel(x, y, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readCeiling(const INFFile &inf)
{
	const INFFile::CeilingData &ceiling = inf.getCeiling();

	// Get the index of the ceiling texture name in the textures array.
	const int ceilingIndex = [&ceiling]()
	{
		// @todo: get ceiling from .INFs without *CEILING (like START.INF). Maybe
		// hardcoding index 1 is enough?
		return ceiling.textureIndex.value_or(1);
	}();

	// Define the ceiling voxel data.
	const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(ceilingIndex);
	TextureAssetReference textureAssetRef(
		ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
		ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
	const int index = this->voxelGrid.addVoxelDef(VoxelDefinition::makeCeiling(std::move(textureAssetRef)));

	// Set all the ceiling voxels.
	const SNInt gridWidth = this->voxelGrid.getWidth();
	const WEInt gridDepth = this->voxelGrid.getDepth();
	for (SNInt x = 0; x < gridWidth; x++)
	{
		for (WEInt z = 0; z < gridDepth; z++)
		{
			this->setVoxel(x, 2, z, index);
		}
	}
}

void LevelData::readLocks(const BufferView<const ArenaTypes::MIFLock> &locks)
{
	for (int i = 0; i < locks.getCount(); i++)
	{
		const auto &lock = locks.get(i);
		const NewInt2 lockPosition = VoxelUtils::originalVoxelToNewVoxel(OriginalInt2(lock.x, lock.y));
		this->locks.insert(std::make_pair(lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}
}

void LevelData::readTriggers(const BufferView<const ArenaTypes::MIFTrigger> &triggers, const INFFile &inf)
{
	DebugAssert(this->isInterior);

	for (int i = 0; i < triggers.getCount(); i++)
	{
		const auto &trigger = triggers.get(i);

		// Transform the voxel coordinates from the Arena layout to the new layout.
		const NewInt2 voxel = VoxelUtils::originalVoxelToNewVoxel(OriginalInt2(trigger.x, trigger.y));

		// There can be a text trigger and sound trigger in the same voxel.
		const bool isTextTrigger = trigger.textIndex != -1;
		const bool isSoundTrigger = trigger.soundIndex != -1;

		// Make sure the text index points to a text value (i.e., not a key or riddle).
		if (isTextTrigger && inf.hasTextIndex(trigger.textIndex))
		{
			const INFFile::TextData &textData = inf.getText(trigger.textIndex);
			this->interior.textTriggers.emplace(std::make_pair(
				voxel, TextTrigger(textData.text, textData.displayedOnce)));
		}

		if (isSoundTrigger)
		{
			this->interior.soundTriggers.emplace(std::make_pair(voxel, inf.getSound(trigger.soundIndex)));
		}
	}
}

void LevelData::getAdjacentVoxelIDs(const NewInt3 &voxel, uint16_t *outNorthID, uint16_t *outSouthID,
	uint16_t *outEastID, uint16_t *outWestID) const
{
	auto getVoxelIdOrAir = [this](const NewInt3 &voxel)
	{
		// The voxel is air if outside the grid.
		return this->voxelGrid.coordIsValid(voxel.x, voxel.y, voxel.z) ?
			this->voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z) : 0;
	};

	const NewInt3 northVoxel(voxel.x - 1, voxel.y, voxel.z);
	const NewInt3 southVoxel(voxel.x + 1, voxel.y, voxel.z);
	const NewInt3 eastVoxel(voxel.x, voxel.y, voxel.z - 1);
	const NewInt3 westVoxel(voxel.x, voxel.y, voxel.z + 1);

	if (outNorthID != nullptr)
	{
		*outNorthID = getVoxelIdOrAir(northVoxel);
	}

	if (outSouthID != nullptr)
	{
		*outSouthID = getVoxelIdOrAir(southVoxel);
	}

	if (outEastID != nullptr)
	{
		*outEastID = getVoxelIdOrAir(eastVoxel);
	}

	if (outWestID != nullptr)
	{
		*outWestID = getVoxelIdOrAir(westVoxel);
	}
}

ArenaLevelUtils::MenuNamesList LevelData::generateBuildingNames(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, ArenaRandom &random, const VoxelGrid &voxelGrid,
	const LevelData::Transitions &transitions, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary)
{
	const auto &exeData = binaryAssetLibrary.getExeData();
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

	uint32_t citySeed = cityDef.citySeed;
	const Int2 localCityPoint = LocationUtils::getLocalCityPoint(citySeed);

	ArenaLevelUtils::MenuNamesList menuNames;

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [&provinceDef, &citySeed, &random, &voxelGrid, &transitions, &textAssetLibrary,
		&exeData, &cityDef, &localCityPoint, &menuNames](ArenaTypes::MenuType menuType)
	{
		if ((menuType == ArenaTypes::MenuType::Equipment) ||
			(menuType == ArenaTypes::MenuType::Temple))
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
		auto tryGenerateBlockName = [menuType, &random, &voxelGrid, &transitions, &menuNames, &seen,
			&hashInSeen, &createTavernName, &createEquipmentName, &createTempleName](SNInt x, WEInt z)
		{
			// See if the current voxel is a *MENU block and matches the target menu type.
			const bool matchesTargetType = [x, z, menuType, &voxelGrid, &transitions]()
			{
				const uint16_t voxelID = voxelGrid.getVoxel(x, 1, z);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				if (voxelDef.type != ArenaTypes::VoxelType::Wall)
				{
					return false;
				}

				const auto iter = transitions.find(NewInt2(x, z));
				if (iter == transitions.end())
				{
					return false;
				}

				const LevelData::Transition &transition = iter->second;
				if (transition.getType() != LevelData::Transition::Type::Menu)
				{
					return false;
				}

				const LevelData::Transition::Menu &transitionMenu = transition.getMenu();
				return ArenaVoxelUtils::getMenuType(transitionMenu.id, MapType::City) == menuType;
			}();

			if (matchesTargetType)
			{
				// Get the *MENU block's display name.
				int hash;
				std::string name;

				if (menuType == ArenaTypes::MenuType::Tavern)
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
				else if (menuType == ArenaTypes::MenuType::Equipment)
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
		if ((menuType == ArenaTypes::MenuType::Temple) &&
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

	generateNames(ArenaTypes::MenuType::Tavern);
	generateNames(ArenaTypes::MenuType::Equipment);
	generateNames(ArenaTypes::MenuType::Temple);
	return menuNames;
}

ArenaLevelUtils::MenuNamesList LevelData::generateWildChunkBuildingNames(const VoxelGrid &voxelGrid,
	const LevelData::Transitions &transitions, const ExeData &exeData)
{
	ArenaLevelUtils::MenuNamesList menuNames;

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [&voxelGrid, &transitions, &exeData, &menuNames](int wildX, int wildY,
		ArenaTypes::MenuType menuType)
	{
		const uint32_t wildChunkSeed = ArenaWildUtils::makeWildChunkSeed(wildX, wildY);

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
		auto tryGenerateBlockName = [&voxelGrid, &transitions, &menuNames, wildX, wildY, menuType,
			wildChunkSeed, &createTavernName, &createTempleName](SNInt x, WEInt z)
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
			const bool matchesTargetType = [&voxelGrid, &transitions, menuType, &dstPoint]()
			{
				const uint16_t voxelID = voxelGrid.getVoxel(dstPoint.x, 1, dstPoint.y);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				if (voxelDef.type != ArenaTypes::VoxelType::Wall)
				{
					return false;
				}

				const auto iter = transitions.find(dstPoint);
				if (iter == transitions.end())
				{
					return false;
				}

				const LevelData::Transition &transition = iter->second;
				if (transition.getType() != LevelData::Transition::Type::Menu)
				{
					return false;
				}

				const LevelData::Transition::Menu &transitionMenu = transition.getMenu();
				constexpr MapType mapType = MapType::Wilderness;
				return ArenaVoxelUtils::getMenuType(transitionMenu.id, mapType) == menuType;
			}();

			if (matchesTargetType)
			{
				// Get the *MENU block's display name.
				const std::string name = [menuType, &random, &createTavernName, &createTempleName]()
				{
					if (menuType == ArenaTypes::MenuType::Tavern)
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

				menuNames.push_back(std::make_pair(dstPoint, std::move(name)));
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
	for (int y = 0; y < ArenaWildUtils::WILD_HEIGHT; y++)
	{
		for (int x = 0; x < ArenaWildUtils::WILD_WIDTH; x++)
		{
			generateNames(x, y, ArenaTypes::MenuType::Tavern);
			generateNames(x, y, ArenaTypes::MenuType::Temple);
		}
	}

	return menuNames;
}

void LevelData::tryUpdateChasmVoxel(const NewInt3 &voxel)
{
	// Ignore if outside the grid.
	if (!this->voxelGrid.coordIsValid(voxel.x, voxel.y, voxel.z))
	{
		return;
	}

	const uint16_t voxelID = this->voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
	const VoxelDefinition &voxelDef = this->voxelGrid.getVoxelDef(voxelID);

	// Ignore if not a chasm (no faces to update).
	if (voxelDef.type != ArenaTypes::VoxelType::Chasm)
	{
		return;
	}

	// Query surrounding voxels to see which faces should be set.
	uint16_t northID, southID, eastID, westID;
	this->getAdjacentVoxelIDs(voxel, &northID, &southID, &eastID, &westID);

	const VoxelDefinition &northDef = this->voxelGrid.getVoxelDef(northID);
	const VoxelDefinition &southDef = this->voxelGrid.getVoxelDef(southID);
	const VoxelDefinition &eastDef = this->voxelGrid.getVoxelDef(eastID);
	const VoxelDefinition &westDef = this->voxelGrid.getVoxelDef(westID);

	// Booleans for each face of the new chasm voxel.
	const bool hasNorthFace = northDef.allowsChasmFace();
	const bool hasSouthFace = southDef.allowsChasmFace();
	const bool hasEastFace = eastDef.allowsChasmFace();
	const bool hasWestFace = westDef.allowsChasmFace();

	// Lambda for creating chasm voxel instance (replaces local variable since this is cleaner
	// with the below if/else branches and it avoids the assertion in the voxel instance builder).
	auto makeChasmInst = [&voxel, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace]()
	{
		return VoxelInstance::makeChasm(voxel.x, voxel.y, voxel.z, hasNorthFace, hasEastFace,
			hasSouthFace, hasWestFace);
	};

	// Add/update chasm state.
	const ChunkInt2 chunk = VoxelUtils::newVoxelToChunk(NewInt2(voxel.x, voxel.z));
	VoxelInstanceGroup *voxelInstGroup = this->tryGetVoxelInstances(chunk);
	const bool shouldAddChasmState = hasNorthFace || hasEastFace || hasSouthFace || hasWestFace;
	if (voxelInstGroup != nullptr)
	{
		auto groupIter = voxelInstGroup->find(voxel);
		if ((groupIter == voxelInstGroup->end()) && shouldAddChasmState)
		{
			groupIter = voxelInstGroup->emplace(std::make_pair(voxel, std::vector<VoxelInstance>())).first;
		}

		if (groupIter != voxelInstGroup->end())
		{
			std::vector<VoxelInstance> &voxelInsts = groupIter->second;
			const auto voxelIter = std::find_if(voxelInsts.begin(), voxelInsts.end(),
				[](const VoxelInstance &inst)
			{
				return inst.getType() == VoxelInstance::Type::Chasm;
			});

			if (voxelIter != voxelInsts.end())
			{
				if (shouldAddChasmState)
				{
					*voxelIter = makeChasmInst();
				}
				else
				{
					voxelInsts.erase(voxelIter);
				}
			}
			else
			{
				if (shouldAddChasmState)
				{
					voxelInsts.emplace_back(makeChasmInst());
				}
			}
		}
	}
	else
	{
		if (shouldAddChasmState)
		{
			this->addVoxelInstance(makeChasmInst());
		}
	}
}

uint16_t LevelData::getChasmIdFromFadedFloorVoxel(const NewInt3 &voxel)
{
	DebugAssert(this->voxelGrid.coordIsValid(voxel.x, voxel.y, voxel.z));

	// Get voxel IDs of adjacent voxels (potentially air).
	uint16_t northID, southID, eastID, westID;
	this->getAdjacentVoxelIDs(voxel, &northID, &southID, &eastID, &westID);

	const VoxelDefinition &northDef = this->voxelGrid.getVoxelDef(northID);
	const VoxelDefinition &southDef = this->voxelGrid.getVoxelDef(southID);
	const VoxelDefinition &eastDef = this->voxelGrid.getVoxelDef(eastID);
	const VoxelDefinition &westDef = this->voxelGrid.getVoxelDef(westID);

	// Booleans for each face of the new chasm voxel.
	const bool hasNorthFace = northDef.allowsChasmFace();
	const bool hasSouthFace = southDef.allowsChasmFace();
	const bool hasEastFace = eastDef.allowsChasmFace();
	const bool hasWestFace = westDef.allowsChasmFace();

	// Based on how the original game behaves, it seems to be the chasm type closest to the player,
	// even dry chasms, that determines what the destroyed floor becomes. This allows for oddities
	// like creating a dry chasm next to lava, which results in continued oddities like having a
	// big difference in chasm depth between the two (depending on ceiling height).
	const ArenaTypes::ChasmType newChasmType = []()
	{
		// @todo: include player position. If there are no chasms to pick from, then default to
		// wet chasm.
		// @todo: getNearestChasmType(const NewInt3 &voxel)
		return ArenaTypes::ChasmType::Wet;
	}();

	const int newTextureID = [this, newChasmType]()
	{
		std::optional<int> chasmIndex;

		switch (newChasmType)
		{
		case ArenaTypes::ChasmType::Dry:
			chasmIndex = this->inf.getDryChasmIndex();
			break;
		case ArenaTypes::ChasmType::Wet:
			chasmIndex = this->inf.getWetChasmIndex();
			break;
		case ArenaTypes::ChasmType::Lava:
			chasmIndex = this->inf.getLavaChasmIndex();
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(newChasmType)));
			break;
		}

		// Default to the first texture if one is not found.
		return chasmIndex.has_value() ? *chasmIndex : 0;
	}();

	const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(newTextureID);
	TextureAssetReference textureAssetRef(
		ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, this->inf),
		ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, this->inf));
	const VoxelDefinition newDef = VoxelDefinition::makeChasm(std::move(textureAssetRef), newChasmType);

	// Find matching chasm voxel definition, adding if missing.
	const std::optional<uint16_t> optChasmID = this->voxelGrid.findVoxelDef(
		[&newDef](const VoxelDefinition &voxelDef)
	{
		if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
		{
			DebugAssert(newDef.type == ArenaTypes::VoxelType::Chasm);
			const VoxelDefinition::ChasmData &newChasmData = newDef.chasm;
			const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;
			return chasmData.matches(newChasmData);
		}
		else
		{
			return false;
		}
	});

	// Lambda for creating chasm voxel instance (replaces local variable since this is cleaner
	// with the below if/else branches and it avoids the assertion in the voxel instance builder).
	auto makeChasmInst = [&voxel, hasNorthFace, hasEastFace, hasSouthFace, hasWestFace]()
	{
		return VoxelInstance::makeChasm(voxel.x, voxel.y, voxel.z, hasNorthFace, hasEastFace,
			hasSouthFace, hasWestFace);
	};

	// Add/update chasm state.
	const ChunkInt2 chunk = VoxelUtils::newVoxelToChunk(NewInt2(voxel.x, voxel.z));
	VoxelInstanceGroup *voxelInstGroup = this->tryGetVoxelInstances(chunk);
	const bool shouldAddChasmState = hasNorthFace || hasEastFace || hasSouthFace || hasWestFace;
	if (voxelInstGroup != nullptr)
	{
		auto groupIter = voxelInstGroup->find(voxel);
		if ((groupIter == voxelInstGroup->end()) && shouldAddChasmState)
		{
			groupIter = voxelInstGroup->emplace(std::make_pair(voxel, std::vector<VoxelInstance>())).first;
		}

		if (groupIter != voxelInstGroup->end())
		{
			std::vector<VoxelInstance> &voxelInsts = groupIter->second;
			const auto iter = std::find_if(voxelInsts.begin(), voxelInsts.end(),
				[&voxel](const VoxelInstance &inst)
			{
				return (inst.getX() == voxel.x) && (inst.getY() == voxel.y) && (inst.getZ() == voxel.z) &&
					(inst.getType() == VoxelInstance::Type::Chasm);
			});

			if (iter != voxelInsts.end())
			{
				if (shouldAddChasmState)
				{
					*iter = makeChasmInst();
				}
				else
				{
					voxelInsts.erase(iter);
				}
			}
			else
			{
				if (shouldAddChasmState)
				{
					voxelInsts.emplace_back(makeChasmInst());
				}
			}
		}
	}
	else
	{
		if (shouldAddChasmState)
		{
			this->addVoxelInstance(makeChasmInst());
		}
	}

	if (optChasmID.has_value())
	{
		return *optChasmID;
	}
	else
	{
		// Need to add a new voxel data to the voxel grid.
		return this->voxelGrid.addVoxelDef(newDef);
	}
}

void LevelData::updateFadingVoxels(const ChunkInt2 &minChunk, const ChunkInt2 &maxChunk, double dt)
{
	std::vector<NewInt3> completedVoxels;

	for (SNInt chunkX = minChunk.x; chunkX <= maxChunk.x; chunkX++)
	{
		for (WEInt chunkZ = minChunk.y; chunkZ <= maxChunk.y; chunkZ++)
		{
			const ChunkInt2 chunk(chunkX, chunkZ);
			VoxelInstanceGroup *voxelInstGroup = this->tryGetVoxelInstances(chunk);
			if (voxelInstGroup != nullptr)
			{
				for (auto &pair : *voxelInstGroup)
				{
					std::vector<VoxelInstance> &voxelInsts = pair.second;

					// Reverse iterate, removing voxels that are done fading out.
					for (int i = static_cast<int>(voxelInsts.size()) - 1; i >= 0; i--)
					{
						VoxelInstance &voxelInst = voxelInsts[i];
						if (voxelInst.getType() == VoxelInstance::Type::Fading)
						{
							voxelInst.update(dt);

							if (!voxelInst.hasRelevantState())
							{
								const NewInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
								completedVoxels.push_back(voxel);

								const uint16_t newVoxelID = [this, &voxel]() -> uint16_t
								{
									const bool isFloorVoxel = voxel.y == 0;
									if (isFloorVoxel)
									{
										// Convert from floor to chasm.
										return this->getChasmIdFromFadedFloorVoxel(voxel);
									}
									else
									{
										// Clear the voxel.
										return 0;
									}
								}();

								// Change the voxel to its empty representation (either air or chasm) and erase
								// the fading voxel from the list.
								voxelGrid.setVoxel(voxel.x, voxel.y, voxel.z, newVoxelID);
								voxelInsts.erase(voxelInsts.begin() + i);
							}
						}
					}
				}
			}
		}
	}

	// Update adjacent chasm faces (not sure why this has to be done after, but it works).
	for (const NewInt3 &voxel : completedVoxels)
	{
		const bool isFloorVoxel = voxel.y == 0;

		if (isFloorVoxel)
		{
			const NewInt3 northVoxel(voxel.x - 1, voxel.y, voxel.z);
			const NewInt3 southVoxel(voxel.x + 1, voxel.y, voxel.z);
			const NewInt3 eastVoxel(voxel.x, voxel.y, voxel.z - 1);
			const NewInt3 westVoxel(voxel.x, voxel.y, voxel.z + 1);
			this->tryUpdateChasmVoxel(northVoxel);
			this->tryUpdateChasmVoxel(southVoxel);
			this->tryUpdateChasmVoxel(eastVoxel);
			this->tryUpdateChasmVoxel(westVoxel);
		}
	}
}

void LevelData::setActive(bool nightLightsAreActive, const WorldData &worldData,
	const ProvinceDefinition &provinceDef, const LocationDefinition &locationDef,
	const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, Random &random, CitizenManager &citizenManager,
	TextureManager &textureManager, Renderer &renderer)
{
	// Clear renderer textures, distant sky, and entities.
	renderer.clearTexturesAndEntityRenderIDs();
	renderer.clearDistantSky();
	this->entityManager.clear();

	// Palette for voxels and flats, required in the renderer so it can conditionally transform
	// certain palette indices for transparency.
	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	// Loads .INF voxel textures into the renderer.
	auto loadVoxelTextures = [this, &textureManager, &renderer, &palette]()
	{
		// Iterate the voxel grid's voxel definitions, get the texture asset reference(s), and allocate
		// textures in the renderer.
		// @todo: avoid allocating duplicate textures (maybe keep a hash set here).
		const int voxelDefCount = this->voxelGrid.getVoxelDefCount();
		for (int i = 0; i < voxelDefCount; i++)
		{
			const VoxelDefinition &voxelDef = this->voxelGrid.getVoxelDef(i);
			const Buffer<TextureAssetReference> textureAssetRefs = voxelDef.getTextureAssetReferences();
			for (int j = 0; j < textureAssetRefs.getCount(); j++)
			{
				const TextureAssetReference &textureAssetRef = textureAssetRefs.get(j);
				if (!renderer.tryCreateVoxelTexture(textureAssetRef, textureManager))
				{
					DebugLogError("Couldn't create voxel texture for \"" + textureAssetRef.filename + "\".");
				}
			}
		}
	};

	// Loads screen-space chasm textures into the renderer.
	auto loadChasmTextures = [this, &textureManager, &renderer, &palette]()
	{
		constexpr int chasmWidth = RCIFile::WIDTH;
		constexpr int chasmHeight = RCIFile::HEIGHT;
		Buffer<uint8_t> chasmBuffer(chasmWidth * chasmHeight);

		// Dry chasm (just a single color).
		chasmBuffer.fill(ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR);
		renderer.addChasmTexture(ArenaTypes::ChasmType::Dry, chasmBuffer.get(),
			chasmWidth, chasmHeight, palette);

		// Lambda for writing an .RCI animation to the renderer.
		auto writeChasmAnim = [&textureManager, &renderer, &palette, chasmWidth, chasmHeight](
			ArenaTypes::ChasmType chasmType, const std::string &rciName)
		{
			const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
				textureManager.tryGetTextureBuilderIDs(rciName.c_str());
			if (!textureBuilderIDs.has_value())
			{
				DebugLogError("Couldn't get texture builder IDs for \"" + rciName + "\".");
				return;
			}

			for (int i = 0; i < textureBuilderIDs->getCount(); i++)
			{
				const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);
				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
				
				DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
				const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
				renderer.addChasmTexture(chasmType, palettedTexture.texels.get(),
					textureBuilder.getWidth(), textureBuilder.getHeight(), palette);
			}
		};

		writeChasmAnim(ArenaTypes::ChasmType::Wet, "WATERANI.RCI");
		writeChasmAnim(ArenaTypes::ChasmType::Lava, "LAVAANI.RCI");
	};

	// Initializes entities from the flat defs list and write their textures to the renderer.
	auto loadEntities = [this, nightLightsAreActive, &worldData, &provinceDef, &locationDef,
		&entityDefLibrary, &charClassLibrary, &binaryAssetLibrary, &random, &citizenManager,
		&textureManager, &renderer, &palette]()
	{
		// See whether the current ruler (if any) is male. This affects the displayed ruler in palaces.
		const std::optional<bool> rulerIsMale = [&locationDef]() -> std::optional<bool>
		{
			if (locationDef.getType() == LocationDefinition::Type::City)
			{
				const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
				return cityDef.rulerIsMale;
			}
			else
			{
				return std::nullopt;
			}
		}();

		const MapType mapType = worldData.getMapType();
		const std::optional<ArenaTypes::InteriorType> interiorType = [&worldData, mapType]()
			-> std::optional<ArenaTypes::InteriorType>
		{
			if (mapType == MapType::Interior)
			{
				const WorldData::Interior &interior = worldData.getInterior();
				return interior.interiorType;
			}
			else
			{
				return std::nullopt;
			}
		}();

		const auto &exeData = binaryAssetLibrary.getExeData();
		for (const auto &flatDef : this->flatsLists)
		{
			const ArenaTypes::FlatIndex flatIndex = flatDef.getFlatIndex();
			const INFFile::FlatData &flatData = this->inf.getFlat(flatIndex);
			const EntityType entityType = ArenaAnimUtils::getEntityTypeFromFlat(flatIndex, this->inf);
			const std::optional<ArenaTypes::ItemIndex> &optItemIndex = flatData.itemIndex;

			bool isFinalBoss;
			const bool isCreature = optItemIndex.has_value() &&
				ArenaAnimUtils::isCreatureIndex(*optItemIndex, &isFinalBoss);
			const bool isHumanEnemy = optItemIndex.has_value() &&
				ArenaAnimUtils::isHumanEnemyIndex(*optItemIndex);

			// Must be at least one instance of the entity for the loop to try and
			// instantiate it and write textures to the renderer.
			DebugAssert(flatDef.getPositions().size() > 0);

			// Add entity animation data. Static entities have only idle animations (and maybe on/off
			// state for lampposts). Dynamic entities have several animation states and directions.
			//auto &entityAnimData = newEntityDef.getAnimationData();
			EntityAnimationDefinition entityAnimDef;
			EntityAnimationInstance entityAnimInst;
			if (entityType == EntityType::Static)
			{
				if (!ArenaAnimUtils::tryMakeStaticEntityAnims(flatIndex, mapType, interiorType,
					rulerIsMale, this->inf, textureManager, &entityAnimDef, &entityAnimInst))
				{
					DebugLogWarning("Couldn't make static entity anims for flat \"" +
						std::to_string(flatIndex) + "\".");
					continue;
				}

				// The entity can only be instantiated if there is at least an idle animation.
				int idleStateIndex;
				if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &idleStateIndex))
				{
					DebugLogWarning("Missing static entity idle anim state for flat \"" +
						std::to_string(flatIndex) + "\".");
					continue;
				}
			}
			else if (entityType == EntityType::Dynamic)
			{
				// Assume that human enemies in level data are male.
				const std::optional<bool> isMale = true;

				if (!ArenaAnimUtils::tryMakeDynamicEntityAnims(flatIndex, isMale, this->inf,
					charClassLibrary, binaryAssetLibrary, textureManager, &entityAnimDef,
					&entityAnimInst))
				{
					DebugLogWarning("Couldn't make dynamic entity anims for flat \"" +
						std::to_string(flatIndex) + "\".");
					continue;
				}

				// Must have at least an idle animation.
				int idleStateIndex;
				if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &idleStateIndex))
				{
					DebugLogWarning("Missing dynamic entity idle anim state for flat \"" +
						std::to_string(flatIndex) + "\".");
					continue;
				}
			}
			else
			{
				DebugCrash("Unrecognized entity type \"" +
					std::to_string(static_cast<int>(entityType)) + "\".");
			}

			// @todo: replace isCreature/etc. with some flatIndex -> EntityDefinition::Type function.
			// - Most likely also need location type, etc. because flatIndex is level-dependent.
			EntityDefinition newEntityDef;
			if (isCreature)
			{
				const ArenaTypes::ItemIndex itemIndex = *optItemIndex;
				const int creatureID = isFinalBoss ?
					ArenaAnimUtils::getFinalBossCreatureID() :
					ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				// @todo: read from EntityDefinitionLibrary instead, and don't make anim def above.
				// Currently these are just going to be duplicates of defs in the library.
				EntityDefinitionLibrary::Key entityDefKey;
				entityDefKey.initCreature(creatureIndex, isFinalBoss);

				EntityDefID entityDefID;
				if (!entityDefLibrary.tryGetDefinitionID(entityDefKey, &entityDefID))
				{
					DebugLogWarning("Couldn't get creature definition " +
						std::to_string(creatureIndex) + " from library.");
					continue;
				}

				newEntityDef = entityDefLibrary.getDefinition(entityDefID);
			}
			else if (isHumanEnemy)
			{
				const bool male = (random.next() % 2) == 0;
				const int charClassID = ArenaAnimUtils::getCharacterClassIndexFromItemIndex(*optItemIndex);
				newEntityDef.initEnemyHuman(male, charClassID, std::move(entityAnimDef));
			}
			else // @todo: handle other entity definition types.
			{
				// Doodad.
				const bool streetLight = ArenaAnimUtils::isStreetLightFlatIndex(flatIndex, mapType);
				const double scale = ArenaAnimUtils::getDimensionModifier(flatData);
				const int lightIntensity = flatData.lightIntensity.has_value() ? *flatData.lightIntensity : 0;

				newEntityDef.initDoodad(flatData.yOffset, scale, flatData.collider,
					flatData.transparent, flatData.ceiling, streetLight, flatData.puddle,
					lightIntensity, std::move(entityAnimDef));
			}

			const bool isStreetlight = (newEntityDef.getType() == EntityDefinition::Type::Doodad) &&
				newEntityDef.getDoodad().streetlight;
			const bool isPuddle = (newEntityDef.getType() == EntityDefinition::Type::Doodad) &&
				newEntityDef.getDoodad().puddle;
			const EntityDefID entityDefID = this->entityManager.addEntityDef(
				std::move(newEntityDef), entityDefLibrary);
			const EntityDefinition &entityDefRef = this->entityManager.getEntityDef(
				entityDefID, entityDefLibrary);
			
			// Quick hack to get back the anim def that was moved into the entity def.
			const EntityAnimationDefinition &entityAnimDefRef = entityDefRef.getAnimDef();

			// Generate render ID for this entity type to share between identical instances.
			const EntityRenderID entityRenderID = renderer.makeEntityRenderID();

			// Initialize each instance of the flat def.
			for (const NewInt2 &position : flatDef.getPositions())
			{
				EntityRef entityRef = this->entityManager.makeEntity(entityType);

				// Using raw entity pointer in this scope for performance due to it currently being
				// impractical to use the ref wrapper when loading the entire wilderness.
				Entity *entityPtr = entityRef.get();

				if (entityType == EntityType::Static)
				{
					StaticEntity *staticEntity = dynamic_cast<StaticEntity*>(entityPtr);
					staticEntity->initDoodad(entityDefID, entityAnimInst);
				}
				else if (entityType == EntityType::Dynamic)
				{
					// All dynamic entities in a level are creatures (never citizens).
					DynamicEntity *dynamicEntity = dynamic_cast<DynamicEntity*>(entityPtr);
					dynamicEntity->initCreature(entityDefID, entityAnimInst,
						CardinalDirection::North, random);
				}
				else
				{
					DebugCrash("Unrecognized entity type \"" +
						std::to_string(static_cast<int>(entityType)) + "\".");
				}

				entityPtr->setRenderID(entityRenderID);

				// Set default animation state.
				int defaultStateIndex;
				if (!isStreetlight)
				{
					// Entities will use idle animation by default.
					if (!entityAnimDefRef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &defaultStateIndex))
					{
						DebugLogWarning("Couldn't get idle state index for flat \"" +
							std::to_string(flatIndex) + "\".");
						continue;
					}
				}
				else
				{
					// Need to turn streetlights on or off at initialization.
					const std::string &streetlightStateName = nightLightsAreActive ?
						EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;

					if (!entityAnimDefRef.tryGetStateIndex(streetlightStateName.c_str(), &defaultStateIndex))
					{
						DebugLogWarning("Couldn't get \"" + streetlightStateName +
							"\" streetlight state index for flat \"" + std::to_string(flatIndex) + "\".");
						continue;
					}
				}

				EntityAnimationInstance &animInst = entityPtr->getAnimInstance();
				animInst.setStateIndex(defaultStateIndex);

				// Note: since the entity pointer is being used directly, update the position last
				// in scope to avoid a dangling pointer problem in case it changes chunks (from 0, 0).
				const NewDouble2 positionXZ = VoxelUtils::getVoxelCenter(position);
				const CoordDouble2 coord = VoxelUtils::newPointToCoord(positionXZ);
				entityPtr->setPosition(coord, this->entityManager, this->voxelGrid);
			}

			// Initialize renderer buffers for the entity animation then populate all textures
			// of the animation.
			renderer.setFlatTextures(entityRenderID, entityAnimDefRef, entityAnimInst, isPuddle, textureManager);
		}

		// Spawn citizens at level start if the conditions are met for the new level.
		const bool isCity = mapType == MapType::City;
		const bool isWild = mapType == MapType::Wilderness;
		if (isCity || isWild)
		{
			citizenManager.spawnCitizens(provinceDef.getRaceID(), this->voxelGrid, this->entityManager,
				locationDef, entityDefLibrary, binaryAssetLibrary, random, textureManager, renderer);
		}
	};

	loadVoxelTextures();
	loadChasmTextures();
	loadEntities();

	// Level-type-specific loading.
	if (this->isInterior)
	{
		renderer.setSkyPalette(&this->interior.skyColor, 1);
	}
	else
	{
		renderer.setDistantSky(this->exterior.distantSky, palette, textureManager);
	}
}

void LevelData::tick(Game &game, double dt)
{
	const int chunkDistance = game.getOptions().getMisc_ChunkDistance();
	const auto &player = game.getGameData().getPlayer();
	const ChunkInt2 &playerChunk = player.getPosition().chunk;
	
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, chunkDistance, &minChunk, &maxChunk);
	this->updateFadingVoxels(minChunk, maxChunk, dt);

	// Update entities.
	this->entityManager.tick(game, dt);

	// Level-type-specific updating.
	if (!this->isInterior)
	{
		this->exterior.distantSky.tick(dt);
	}
}
