#include "InteriorLevelData.h"
#include "InteriorLevelUtils.h"
#include "WorldType.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/String.h"

InteriorLevelData::InteriorLevelData(SNInt gridWidth, WEInt gridDepth, const std::string &infName,
	const std::string &name)
	: LevelData(gridWidth, InteriorLevelUtils::GRID_HEIGHT, gridDepth, infName, name) { }

InteriorLevelData::~InteriorLevelData()
{

}

InteriorLevelData InteriorLevelData::loadInterior(const MIFFile::Level &level, SNInt gridWidth,
	WEInt gridDepth, const ExeData &exeData)
{
	// .INF filename associated with the interior level.
	const std::string infName = String::toUppercase(level.getInfo());

	// Interior level.
	InteriorLevelData levelData(gridWidth, gridDepth, infName, level.getName());

	const INFFile &inf = levelData.getInfFile();
	levelData.outdoorDungeon = inf.getCeiling().outdoorDungeon;

	// Interior sky color (usually black, but also gray for "outdoor" dungeons).
	// @todo: use actual colors from palette.
	levelData.skyColor = levelData.isOutdoorDungeon() ?
		Color::Gray.toARGB() : Color::Black.toARGB();

	// Load FLOR and MAP1 voxels.
	levelData.readFLOR(level.getFLOR(), inf);
	levelData.readMAP1(level.getMAP1(), inf, WorldType::Interior, exeData);

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

InteriorLevelData InteriorLevelData::loadDungeon(ArenaRandom &random,
	const MIFFile &mif, int levelUpBlock, const int *levelDownBlock, int widthChunks,
	int depthChunks, const std::string &infName, SNInt gridWidth, WEInt gridDepth,
	const ExeData &exeData)
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
		const SNInt zOffset = row * InteriorLevelUtils::DUNGEON_CHUNK_DIM;
		for (WEInt column = 0; column < widthChunks; column++)
		{
			const WEInt xOffset = column * InteriorLevelUtils::DUNGEON_CHUNK_DIM;

			// Get the selected level from the .MIF file.
			const int blockIndex = (tileSet * 8) + (random.next() % 8);
			const auto &blockLevel = mif.getLevel(blockIndex);
			const BufferView2D<const MIFFile::VoxelID> &blockFLOR = blockLevel.getFLOR();
			const BufferView2D<const MIFFile::VoxelID> &blockMAP1 = blockLevel.getMAP1();

			// Copy block data to temp buffers.
			for (SNInt z = 0; z < InteriorLevelUtils::DUNGEON_CHUNK_DIM; z++)
			{
				for (WEInt x = 0; x < InteriorLevelUtils::DUNGEON_CHUNK_DIM; x++)
				{
					const MIFFile::VoxelID srcFlorVoxel = blockFLOR.get(x, z);
					const MIFFile::VoxelID srcMap1Voxel = blockMAP1.get(x, z);
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
	InteriorLevelData levelData(gridWidth, gridDepth, infName, std::string());
	levelData.outdoorDungeon = false;

	// Draw perimeter blocks. First top and bottom, then right and left.
	constexpr MIFFile::VoxelID perimeterVoxel = 0x7800;
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
	InteriorLevelUtils::unpackLevelChangeVoxel(levelUpBlock, &levelUpX, &levelUpZ);
	tempMap1.set(InteriorLevelUtils::offsetLevelChangeVoxel(levelUpX),
		InteriorLevelUtils::offsetLevelChangeVoxel(levelUpZ),
		InteriorLevelUtils::convertLevelChangeVoxel(levelUpVoxelByte));

	if (levelDownBlock != nullptr)
	{
		const uint8_t levelDownVoxelByte = *inf.getLevelDownIndex() + 1;
		WEInt levelDownX;
		SNInt levelDownZ;
		InteriorLevelUtils::unpackLevelChangeVoxel(*levelDownBlock, &levelDownX, &levelDownZ);
		tempMap1.set(InteriorLevelUtils::offsetLevelChangeVoxel(levelDownX),
			InteriorLevelUtils::offsetLevelChangeVoxel(levelDownZ),
			InteriorLevelUtils::convertLevelChangeVoxel(levelDownVoxelByte));
	}

	// Interior sky color (always black for dungeons).
	// @todo: use actual color from palette.
	levelData.skyColor = Color::Black.toARGB();

	const BufferView2D<const MIFFile::VoxelID> tempFlorView(
		tempFlor.get(), tempFlor.getWidth(), tempFlor.getHeight());
	const BufferView2D<const MIFFile::VoxelID> tempMap1View(
		tempMap1.get(), tempMap1.getWidth(), tempMap1.getHeight());

	// Load FLOR, MAP1, and ceiling into the voxel grid.
	levelData.readFLOR(tempFlorView, inf);
	levelData.readMAP1(tempMap1View, inf, WorldType::Interior, exeData);
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

LevelData::TextTrigger *InteriorLevelData::getTextTrigger(const NewInt2 &voxel)
{
	const auto textIter = this->textTriggers.find(voxel);
	return (textIter != this->textTriggers.end()) ? &textIter->second : nullptr;
}

const std::string *InteriorLevelData::getSoundTrigger(const NewInt2 &voxel) const
{
	const auto soundIter = this->soundTriggers.find(voxel);
	return (soundIter != this->soundTriggers.end()) ? &soundIter->second : nullptr;
}

bool InteriorLevelData::isOutdoorDungeon() const
{
	return this->outdoorDungeon;
}

void InteriorLevelData::readTriggers(const BufferView<const ArenaTypes::MIFTrigger> &triggers,
	const INFFile &inf)
{
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
	const ProvinceDefinition &provinceDef, const LocationDefinition &locationDef,
	const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, Random &random, CitizenManager &citizenManager,
	TextureManager &textureManager, TextureInstanceManager &textureInstManager, Renderer &renderer)
{
	LevelData::setActive(nightLightsAreActive, worldData, provinceDef, locationDef,
		entityDefLibrary, charClassLibrary, binaryAssetLibrary, random, citizenManager, textureManager,
		textureInstManager, renderer);

	// Set interior sky color.
	renderer.setSkyPalette(&this->skyColor, 1);
}
