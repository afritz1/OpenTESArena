#include "InteriorLevelData.h"
#include "VoxelUtils.h"
#include "WorldType.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/String.h"

const int InteriorLevelData::GRID_HEIGHT = 3;

InteriorLevelData::InteriorLevelData(int gridWidth, int gridDepth, const std::string &infName,
	const std::string &name)
	: LevelData(gridWidth, InteriorLevelData::GRID_HEIGHT, gridDepth, infName, name) { }

InteriorLevelData::~InteriorLevelData()
{

}

InteriorLevelData InteriorLevelData::loadInterior(const MIFFile::Level &level, int gridWidth,
	int gridDepth, const ExeData &exeData)
{
	// .INF filename associated with the interior level.
	const std::string infName = String::toUppercase(level.info);

	// Interior level.
	InteriorLevelData levelData(gridWidth, gridDepth, infName, level.name);

	const INFFile &inf = levelData.getInfFile();
	levelData.outdoorDungeon = inf.getCeiling().outdoorDungeon;

	// Interior sky color (usually black, but also gray for "outdoor" dungeons).
	// @todo: use actual colors from palette.
	levelData.skyColor = levelData.isOutdoorDungeon() ?
		Color::Gray.toARGB() : Color::Black.toARGB();

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelDef(VoxelDefinition());

	// Load FLOR and MAP1 voxels.
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1.data(), inf, WorldType::Interior, gridWidth, gridDepth, exeData);

	// All interiors have ceilings except some main quest dungeons which have a 1
	// as the third number after *CEILING in their .INF file.
	const bool hasCeiling = !inf.getCeiling().outdoorDungeon;

	// Fill the second floor with ceiling tiles if it's an "indoor dungeon". Otherwise,
	// leave it empty (for some "outdoor dungeons").
	if (hasCeiling)
	{
		levelData.readCeiling(inf, gridWidth, gridDepth);
	}

	// Assign locks.
	levelData.readLocks(level.lock, gridWidth, gridDepth);

	// Assign text and sound triggers.
	levelData.readTriggers(level.trig, inf, gridWidth, gridDepth);

	return levelData;
}

InteriorLevelData InteriorLevelData::loadDungeon(ArenaRandom &random,
	const std::vector<MIFFile::Level> &levels, int levelUpBlock, const int *levelDownBlock,
	int widthChunks, int depthChunks, const std::string &infName, int gridWidth, int gridDepth,
	const ExeData &exeData)
{
	// Create temp buffers for dungeon block data.
	std::vector<uint16_t> tempFlor(gridWidth * gridDepth, 0);
	std::vector<uint16_t> tempMap1(gridWidth * gridDepth, 0);
	std::vector<ArenaTypes::MIFLock> tempLocks;
	std::vector<ArenaTypes::MIFTrigger> tempTriggers;

	const int chunkDim = 32;
	const int tileSet = random.next() % 4;

	for (int row = 0; row < depthChunks; row++)
	{
		const int dZ = row * chunkDim;
		for (int column = 0; column < widthChunks; column++)
		{
			const int dX = column * chunkDim;

			// Get the selected level from the .MIF file.
			const int blockIndex = (tileSet * 8) + (random.next() % 8);
			const auto &blockLevel = levels.at(blockIndex);

			// Copy block data to temp buffers.
			for (int z = 0; z < chunkDim; z++)
			{
				const int srcIndex = z * chunkDim;
				const int dstIndex = dX + ((z + dZ) * gridDepth);

				auto writeRow = [chunkDim, srcIndex, dstIndex](
					const std::vector<uint16_t> &src, std::vector<uint16_t> &dst)
				{
					const auto srcBegin = src.begin() + srcIndex;
					const auto srcEnd = srcBegin + chunkDim;
					const auto dstBegin = dst.begin() + dstIndex;
					std::copy(srcBegin, srcEnd, dstBegin);
				};

				writeRow(blockLevel.flor, tempFlor);
				writeRow(blockLevel.map1, tempMap1);
			}

			// Assign locks to the current block.
			for (const auto &lock : blockLevel.lock)
			{
				ArenaTypes::MIFLock tempLock;
				tempLock.x = lock.x + dX;
				tempLock.y = lock.y + dZ;
				tempLock.lockLevel = lock.lockLevel;

				tempLocks.push_back(std::move(tempLock));
			}

			// Assign text/sound triggers to the current block.
			for (const auto &trigger : blockLevel.trig)
			{
				ArenaTypes::MIFTrigger tempTrigger;
				tempTrigger.x = trigger.x + dX;
				tempTrigger.y = trigger.y + dZ;
				tempTrigger.textIndex = trigger.textIndex;
				tempTrigger.soundIndex = trigger.soundIndex;

				tempTriggers.push_back(std::move(tempTrigger));
			}
		}
	}

	// Dungeon (either named or in wilderness).
	InteriorLevelData levelData(gridWidth, gridDepth, infName, std::string());
	levelData.outdoorDungeon = false;

	// Draw perimeter blocks. First top and bottom, then right and left.
	const uint16_t perimeterVoxel = 0x7800;
	std::fill(tempMap1.begin(), tempMap1.begin() + gridDepth, perimeterVoxel);
	std::fill(tempMap1.rbegin(), tempMap1.rbegin() + gridDepth, perimeterVoxel);

	for (int z = 1; z < (gridWidth - 1); z++)
	{
		tempMap1.at(z * gridDepth) = perimeterVoxel;
		tempMap1.at((z * gridDepth) + (gridDepth - 1)) = perimeterVoxel;
	}

	const INFFile &inf = levelData.getInfFile();

	// Put transition blocks, unless null. Unpack the level up/down block indices
	// into X and Z chunk offsets.
	const uint8_t levelUpVoxelByte = *inf.getLevelUpIndex() + 1;
	const int levelUpX = 10 + ((levelUpBlock % 10) * chunkDim);
	const int levelUpZ = 10 + ((levelUpBlock / 10) * chunkDim);
	tempMap1.at(levelUpX + (levelUpZ * gridDepth)) = (levelUpVoxelByte << 8) | levelUpVoxelByte;

	if (levelDownBlock != nullptr)
	{
		const uint8_t levelDownVoxelByte = *inf.getLevelDownIndex() + 1;
		const int levelDownX = 10 + ((*levelDownBlock % 10) * chunkDim);
		const int levelDownZ = 10 + ((*levelDownBlock / 10) * chunkDim);
		tempMap1.at(levelDownX + (levelDownZ * gridDepth)) =
			(levelDownVoxelByte << 8) | levelDownVoxelByte;
	}

	// Interior sky color (always black for dungeons).
	// @todo: use actual color from palette.
	levelData.skyColor = Color::Black.toARGB();

	// Empty voxel data (for air).
	levelData.getVoxelGrid().addVoxelDef(VoxelDefinition());

	// Load FLOR, MAP1, and ceiling into the voxel grid.
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, WorldType::Interior, gridWidth, gridDepth, exeData);
	levelData.readCeiling(inf, gridWidth, gridDepth);

	// Load locks and triggers (if any).
	levelData.readLocks(tempLocks, gridWidth, gridDepth);
	levelData.readTriggers(tempTriggers, inf, gridWidth, gridDepth);

	return levelData;
}

LevelData::TextTrigger *InteriorLevelData::getTextTrigger(const Int2 &voxel)
{
	const auto textIter = this->textTriggers.find(voxel);
	return (textIter != this->textTriggers.end()) ? &textIter->second : nullptr;
}

const std::string *InteriorLevelData::getSoundTrigger(const Int2 &voxel) const
{
	const auto soundIter = this->soundTriggers.find(voxel);
	return (soundIter != this->soundTriggers.end()) ? &soundIter->second : nullptr;
}

bool InteriorLevelData::isOutdoorDungeon() const
{
	return this->outdoorDungeon;
}

void InteriorLevelData::readTriggers(const std::vector<ArenaTypes::MIFTrigger> &triggers,
	const INFFile &inf, int width, int depth)
{
	for (const auto &trigger : triggers)
	{
		// Transform the voxel coordinates from the Arena layout to the new layout.
		const NewInt2 voxel = VoxelUtils::originalVoxelToNewVoxel(
			OriginalInt2(trigger.x, trigger.y), width, depth);

		// There can be a text trigger and sound trigger in the same voxel.
		const bool isTextTrigger = trigger.textIndex != -1;
		const bool isSoundTrigger = trigger.soundIndex != -1;

		// Make sure the text index points to a text value (i.e., not a key or riddle).
		if (isTextTrigger && inf.hasTextIndex(trigger.textIndex))
		{
			const INFFile::TextData &textData = inf.getText(trigger.textIndex);
			this->textTriggers.insert(std::make_pair(
				voxel, TextTrigger(textData.text, textData.displayedOnce)));
		}

		if (isSoundTrigger)
		{
			this->soundTriggers.insert(std::make_pair(voxel, inf.getSound(trigger.soundIndex)));
		}
	}
}

void InteriorLevelData::setActive(bool nightLightsAreActive, const WorldData &worldData,
	const Location &location, const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	LevelData::setActive(nightLightsAreActive, worldData, location, miscAssets,
		textureManager, renderer);

	// Set interior sky color.
	renderer.setSkyPalette(&this->skyColor, 1);
}
