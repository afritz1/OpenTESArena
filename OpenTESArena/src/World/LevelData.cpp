#include <algorithm>
#include <cassert>
#include <functional>
#include <sstream>

#include "LevelData.h"
#include "../Assets/INFFile.h"
#include "../Assets/RMDFile.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/VoxelType.h"

LevelData::Lock::Lock(const Int2 &position, int lockLevel)
	: position(position)
{
	this->lockLevel = lockLevel;
}

LevelData::Lock::~Lock()
{

}

const Int2 &LevelData::Lock::getPosition() const
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

LevelData::TextTrigger::~TextTrigger()
{

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

LevelData::LevelData(int gridWidth, int gridHeight, int gridDepth)
	: voxelGrid(gridWidth, gridHeight, gridDepth)
{
	// Just for initializing grid dimensions. The rest is initialized by load methods.
}

LevelData::LevelData(VoxelGrid &&voxelGrid)
	: voxelGrid(std::move(voxelGrid))
{
	this->ceilingHeight = 1.0;
}

LevelData::~LevelData()
{

}

LevelData LevelData::loadInterior(const MIFFile::Level &level, int gridWidth, int gridDepth)
{
	// .INF file associated with the interior level.
	const INFFile inf(String::toUppercase(level.info));

	// Interior level.
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = static_cast<double>(inf.getCeiling().height) / MIFFile::ARENA_UNITS;
	levelData.outdoorDungeon = inf.getCeiling().outdoorDungeon;

	// Interior sky color (usually black, but also gray for "outdoor" dungeons).
	levelData.interiorSkyColor = std::make_unique<uint32_t>(
		levelData.isOutdoorDungeon() ? Color::Gray.toARGB() : Color::Black.toARGB());

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR and MAP1 voxels.
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1.data(), inf, gridWidth, gridDepth);

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

LevelData LevelData::loadDungeon(ArenaRandom &random, const std::vector<MIFFile::Level> &levels,
	int levelUpBlock, const int *levelDownBlock, int widthChunks, int depthChunks,
	const INFFile &inf, int gridWidth, int gridDepth)
{
	// Create temp buffers for dungeon voxel data.
	std::vector<uint16_t> tempFlor(gridWidth * gridDepth, 0);
	std::vector<uint16_t> tempMap1(gridWidth * gridDepth, 0);

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

			// To do: Assign locks?

			// To do: Assign text/sound triggers.
		}
	}

	// Draw perimeter blocks. First top and bottom, then right and left.
	const uint16_t perimeterVoxel = 0x7800;
	std::fill(tempMap1.begin(), tempMap1.begin() + gridDepth, perimeterVoxel);
	std::fill(tempMap1.rbegin(), tempMap1.rbegin() + gridDepth, perimeterVoxel);

	for (int z = 1; z < (gridWidth - 1); z++)
	{
		tempMap1.at(z * gridDepth) = perimeterVoxel;
		tempMap1.at((z * gridDepth) + (gridDepth - 1)) = perimeterVoxel;
	}

	// To do: Put transition blocks, unless null.

	// Dungeon (either named or in wilderness).
	LevelData levelData(gridWidth, 3, gridDepth);
	levelData.infName = inf.getName();
	levelData.ceilingHeight = static_cast<double>(inf.getCeiling().height) / MIFFile::ARENA_UNITS;
	levelData.outdoorDungeon = false;

	// Interior sky color (always black for dungeons).
	levelData.interiorSkyColor = std::make_unique<uint32_t>(Color::Black.toARGB());
	
	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and ceiling into the voxel grid.
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, gridWidth, gridDepth);
	levelData.readCeiling(inf, gridWidth, gridDepth);

	return levelData;
}

LevelData LevelData::loadPremadeCity(const MIFFile::Level &level, const INFFile &inf,
	int gridWidth, int gridDepth)
{
	// Premade exterior level (only used by center province).
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = 1.0;
	levelData.outdoorDungeon = false;

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	levelData.readFLOR(level.flor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1.data(), inf, gridWidth, gridDepth);
	levelData.readMAP2(level.map2.data(), inf, gridWidth, gridDepth);

	return levelData;
}

LevelData LevelData::loadCity(const MIFFile::Level &level, int cityX, int cityY,
	int cityDim, const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
	const INFFile &inf, int gridWidth, int gridDepth)
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
	const uint32_t citySeed = static_cast<uint32_t>(cityX << 16) + cityY;
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

			// To do: load flats.
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
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = 1.0;
	levelData.outdoorDungeon = false;

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, gridWidth, gridDepth);
	levelData.readMAP2(tempMap2.data(), inf, gridWidth, gridDepth);

	return levelData;
}

LevelData LevelData::loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL, const INFFile &inf)
{
	// Load WILD.MIF (blank slate, to be filled in by four .RMD files).
	const MIFFile mif("WILD.MIF");
	const MIFFile::Level &level = mif.getLevels().front();
	const int gridWidth = 128;
	const int gridDepth = gridWidth;

	// Copy voxel data into temp buffers. Each floor in the four 64x64 wilderness blocks
	// is 8192 bytes.
	std::array<uint16_t, 4096 * 4> tempFlor, tempMap1, tempMap2;
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

			auto writeRow = [srcIndex, dstIndex](const RMDFile::ArrayType &src,
				std::array<uint16_t, 4096 * 4> &dst)
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
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = 1.0;
	levelData.outdoorDungeon = false;

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels into the voxel grid.
	levelData.readFLOR(tempFlor.data(), inf, gridWidth, gridDepth);
	levelData.readMAP1(tempMap1.data(), inf, gridWidth, gridDepth);
	levelData.readMAP2(tempMap2.data(), inf, gridWidth, gridDepth);
	// To do: load FLAT from WILD.MIF level data. levelData.readFLAT(level.flat, ...)?

	return levelData;
}

bool LevelData::isOutdoorDungeon() const
{
	return this->outdoorDungeon;
}

double LevelData::getCeilingHeight() const
{
	return this->ceilingHeight;
}

const std::string &LevelData::getName() const
{
	return this->name;
}

const std::string &LevelData::getInfName() const
{
	return this->infName;
}

uint32_t LevelData::getInteriorSkyColor() const
{
	assert(this->interiorSkyColor.get() != nullptr);
	return *this->interiorSkyColor.get();
}

VoxelGrid &LevelData::getVoxelGrid()
{
	return this->voxelGrid;
}

const VoxelGrid &LevelData::getVoxelGrid() const
{
	return this->voxelGrid;
}

const LevelData::Lock *LevelData::getLock(const Int2 &voxel) const
{
	const auto lockIter = this->locks.find(voxel);
	return (lockIter != this->locks.end()) ? (&lockIter->second) : nullptr;
}

LevelData::TextTrigger *LevelData::getTextTrigger(const Int2 &voxel)
{
	const auto textIter = this->textTriggers.find(voxel);
	return (textIter != this->textTriggers.end()) ? (&textIter->second) : nullptr;
}

const std::string *LevelData::getSoundTrigger(const Int2 &voxel) const
{
	const auto soundIter = this->soundTriggers.find(voxel);
	return (soundIter != this->soundTriggers.end()) ? (&soundIter->second) : nullptr;
}

void LevelData::setVoxel(int x, int y, int z, uint16_t id)
{
	uint16_t *voxels = this->voxelGrid.getVoxels();
	const int index = x + (y * this->voxelGrid.getWidth()) +
		(z * this->voxelGrid.getWidth() * this->voxelGrid.getHeight());

	voxels[index] = id;
}

void LevelData::readFLOR(const uint16_t *flor, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte FLOR voxel.
	auto getFlorVoxel = [flor, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(flor) + index);
		return voxel;
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			auto getFloorTextureID = [](uint16_t voxel)
			{
				return (voxel & 0xFF00) >> 8;
			};

			auto isChasm = [](int id)
			{
				return (id == MIFFile::DRY_CHASM) ||
					(id == MIFFile::LAVA_CHASM) ||
					(id == MIFFile::WET_CHASM);
			};

			const uint16_t florVoxel = getFlorVoxel(x, z);
			const int floorTextureID = getFloorTextureID(florVoxel);

			// See if the floor voxel is either solid or a chasm.
			if (!isChasm(floorTextureID))
			{
				// Get the voxel data index associated with the floor value, or add it
				// if it doesn't exist yet.
				const int dataIndex = [this, florVoxel, floorTextureID]()
				{
					const auto floorIter = this->floorDataMappings.find(florVoxel);
					if (floorIter != this->floorDataMappings.end())
					{
						return floorIter->second;
					}
					else
					{
						const int index = this->voxelGrid.addVoxelData(
							VoxelData::makeFloor(floorTextureID));
						return this->floorDataMappings.insert(
							std::make_pair(florVoxel, index)).first->second;
					}
				}();

				this->setVoxel(x, 0, z, dataIndex);
			}
			else
			{
				// The voxel is a chasm. See which of its four faces are adjacent to
				// a solid floor voxel.
				const uint16_t northVoxel = getFlorVoxel(std::min(x + 1, gridWidth - 1), z);
				const uint16_t eastVoxel = getFlorVoxel(x, std::min(z + 1, gridDepth - 1));
				const uint16_t southVoxel = getFlorVoxel(std::max(x - 1, 0), z);
				const uint16_t westVoxel = getFlorVoxel(x, std::max(z - 1, 0));

				const std::array<bool, 4> adjacentFaces
				{
					!isChasm(getFloorTextureID(northVoxel)), // North.
					!isChasm(getFloorTextureID(eastVoxel)), // East.
					!isChasm(getFloorTextureID(southVoxel)), // South.
					!isChasm(getFloorTextureID(westVoxel)) // West.
				};

				// Lambda for obtaining the index of a newly-added VoxelData object, and
				// inserting it into the chasm data mappings if it hasn't been already. The
				// function parameter decodes the voxel and returns the created VoxelData.
				auto getChasmDataIndex = [this, &inf, florVoxel, &adjacentFaces](
					const std::function<VoxelData(void)> &function)
				{
					const auto chasmPair = std::make_pair(florVoxel, adjacentFaces);
					const auto chasmIter = this->chasmDataMappings.find(chasmPair);
					if (chasmIter != this->chasmDataMappings.end())
					{
						return chasmIter->second;
					}
					else
					{
						const int index = this->voxelGrid.addVoxelData(function());
						return this->chasmDataMappings.insert(
							std::make_pair(chasmPair, index)).first->second;
					}
				};

				if (floorTextureID == MIFFile::DRY_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int dryChasmID = [&inf]()
						{
							const int *ptr = inf.getDryChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *DRYCHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							dryChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Dry);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::LAVA_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int lavaChasmID = [&inf]()
						{
							const int *ptr = inf.getLavaChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *LAVACHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							lavaChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Lava);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::WET_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int wetChasmID = [&inf]()
						{
							const int *ptr = inf.getWetChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *WETCHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							wetChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Wet);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP1(const uint16_t *map1, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [map1, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map1) + index);
		return voxel;
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			const uint16_t map1Voxel = getMap1Voxel(x, z);

			// Lambda for obtaining the index of a newly-added VoxelData object, and inserting
			// it into the data mappings if it hasn't been already. The function parameter
			// decodes the voxel and returns the created VoxelData.
			auto getDataIndex = [this, &inf, map1Voxel](
				const std::function<VoxelData(void)> &function)
			{
				const auto wallIter = this->wallDataMappings.find(map1Voxel);
				if (wallIter != this->wallDataMappings.end())
				{
					return wallIter->second;
				}
				else
				{
					const int index = this->voxelGrid.addVoxelData(function());
					return this->wallDataMappings.insert(
						std::make_pair(map1Voxel, index)).first->second;
				}
			};

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
						const int dataIndex = getDataIndex([&inf, mostSigByte]()
						{
							const int textureIndex = mostSigByte - 1;

							// Menu index if the voxel has the *MENU tag, or -1 if it is
							// not a *MENU voxel.
							const int menuIndex = inf.getMenuIndex(textureIndex);
							const bool isMenu = menuIndex != -1;

							// Determine what the type of the voxel is (level up/down, menu,
							// transition, etc.).
							const VoxelType type = [&inf, textureIndex, isMenu]()
							{
								// Returns whether the given index pointer is non-null and
								// matches the current texture index.
								auto matchesIndex = [textureIndex](const int *index)
								{
									return (index != nullptr) && (*index == textureIndex);
								};

								if (matchesIndex(inf.getLevelUpIndex()))
								{
									return VoxelType::LevelUp;
								}
								else if (matchesIndex(inf.getLevelDownIndex()))
								{
									return VoxelType::LevelDown;
								}
								else if (isMenu)
								{
									return VoxelType::Menu;
								}
								else
								{
									return VoxelType::Solid;
								}
							}();

							VoxelData voxelData = VoxelData::makeWall(
								textureIndex, textureIndex, textureIndex, type);

							// Set the *MENU index if it's a menu voxel.
							if (isMenu)
							{
								VoxelData::WallData &wallData = voxelData.wall;
								wallData.menuID = menuIndex;
							}

							return voxelData;
						});

						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = getDataIndex([&inf, map1Voxel, mostSigByte]()
						{
							const uint8_t wallTextureID = map1Voxel & 0x000F;
							const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

							const int sideID = [&inf, wallTextureID]()
							{
								const int *ptr = inf.getBoxSide(wallTextureID);
								if (ptr != nullptr)
								{
									return *ptr;
								}
								else
								{
									DebugWarning("Missing *BOXSIDE ID \"" +
										std::to_string(wallTextureID) + "\".");
									return 0;
								}
							}();

							const int floorID = [&inf]()
							{
								const int id = inf.getCeiling().textureIndex;

								if (id >= 0)
								{
									return id;
								}
								else
								{
									DebugWarning("Invalid platform floor ID \"" +
										std::to_string(id) + "\".");
									return 0;
								}
							}();

							const int ceilingID = [&inf, capTextureID]()
							{
								const int *ptr = inf.getBoxCap(capTextureID);
								if (ptr != nullptr)
								{
									return *ptr;
								}
								else
								{
									DebugWarning("Missing *BOXCAP ID \"" +
										std::to_string(capTextureID) + "\".");
									return 0;
								}
							}();

							// To do: The height appears to be some fraction of 64, and 
							// when it's greater than 64, then that determines the offset?
							const double platformHeight = static_cast<double>(mostSigByte) /
								static_cast<double>(MIFFile::ARENA_UNITS);

							const double yOffset = 0.0;
							const double ySize = platformHeight;

							// To do: Clamp top V coordinate positive until the correct platform 
							// height calculation is figured out. Maybe the platform height
							// needs to be multiplied by the ratio between the current ceiling
							// height and the default ceiling height (128)? I.e., multiply by
							// "ceilingHeight"?
							const double vTop = std::max(0.0, 1.0 - platformHeight);
							const double vBottom = Constants::JustBelowOne; // To do: should also be a function.

							return VoxelData::makeRaised(sideID, floorID, ceilingID,
								yOffset, ySize, vTop, vBottom);
						});

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
					const uint8_t flatIndex = map1Voxel & 0x00FF;
					// To do.
				}
				else if (mostSigNibble == 0x9)
				{
					// Transparent block with 1-sided texture on all sides, such as wooden 
					// arches in dungeons. These do not have back-faces (especially when 
					// standing in the voxel itself).
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x00FF) - 1;
						const bool collider = (map1Voxel & 0x0100) == 0;
						return VoxelData::makeTransparentWall(textureIndex, collider);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int textureIndex = (map1Voxel & 0x003F) - 1;

					// It is clamped non-negative due to a case in IMPERIAL.MIF where one temple
					// voxel has all zeroes for its texture index, and it appears solid gray
					// in the original game (presumably a silent bug).
					if (textureIndex >= 0)
					{
						const int dataIndex = getDataIndex([map1Voxel, textureIndex]()
						{
							const double yOffset =
								static_cast<double>((map1Voxel & 0x0E00) >> 8) / 7.0;
							const bool collider = (map1Voxel & 0x0100) != 0;

							const VoxelData::Facing facing = [map1Voxel]()
							{
								// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
								// and C is east. It is stored in two bits above the texture index.
								const int orientation = (map1Voxel & 0x00C0) >> 4;
								if (orientation == 0x0)
								{
									return VoxelData::Facing::PositiveX;
								}
								else if (orientation == 0x4)
								{
									return VoxelData::Facing::NegativeZ;
								}
								else if (orientation == 0x8)
								{
									return VoxelData::Facing::NegativeX;
								}
								else
								{
									return VoxelData::Facing::PositiveZ;
								}
							}();

							return VoxelData::makeEdge(textureIndex, yOffset, collider, facing);
						});

						this->setVoxel(x, 1, z, dataIndex);
					}
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x003F) - 1;
						const VoxelData::DoorData::Type doorType = [map1Voxel]()
						{
							const int type = (map1Voxel & 0x00C0) >> 4;
							if (type == 0x0)
							{
								return VoxelData::DoorData::Type::Swinging;
							}
							else if (type == 0x4)
							{
								return VoxelData::DoorData::Type::Sliding;
							}
							else if (type == 0x8)
							{
								return VoxelData::DoorData::Type::Raising;
							}
							else
							{
								// I don't believe any doors in Arena split (but they are
								// supported by the engine).
								throw std::runtime_error("Bad door type \"" +
									std::to_string(type) + "\".");
							}
						}();

						return VoxelData::makeDoor(textureIndex, doorType);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown.
					DebugWarning("Voxel type 0xC not implemented.");
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall. Its type is determined by the nineth bit.
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x00FF) - 1;
						const bool isRightDiag = (map1Voxel & 0x0100) == 0;
						return VoxelData::makeDiagonal(textureIndex, isRightDiag);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const uint16_t *map2, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte MAP2 voxel.
	auto getMap2Voxel = [map2, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map2) + index);
		return voxel;
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			const uint16_t map2Voxel = getMap2Voxel(x, z);

			if (map2Voxel != 0)
			{
				// Number of blocks to extend upwards (including second story).
				const int height = [map2Voxel]()
				{
					if ((map2Voxel & 0x80) == 0x80)
					{
						return 2;
					}
					else if ((map2Voxel & 0x8000) == 0x8000)
					{
						return 3;
					}
					else if ((map2Voxel & 0x8080) == 0x8080)
					{
						return 4;
					}
					else
					{
						return 1;
					}
				}();

				const int dataIndex = [this, &inf, map2Voxel, height]()
				{
					const auto map2Iter = this->map2DataMappings.find(map2Voxel);
					if (map2Iter != this->map2DataMappings.end())
					{
						return map2Iter->second;
					}
					else
					{
						const int textureIndex = (map2Voxel & 0x007F) - 1;
						const int index = this->voxelGrid.addVoxelData(VoxelData::makeWall(
							textureIndex, textureIndex, textureIndex, VoxelType::Solid));
						return this->map2DataMappings.insert(
							std::make_pair(map2Voxel, index)).first->second;
					}
				}();

				for (int y = 2; y < (height + 2); y++)
				{
					this->setVoxel(x, y, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readCeiling(const INFFile &inf, int width, int depth)
{
	const INFFile::CeilingData &ceiling = inf.getCeiling();

	// Get the index of the ceiling texture name in the textures array.
	const int ceilingIndex = [&ceiling]()
	{
		if (ceiling.textureIndex != INFFile::NO_INDEX)
		{
			return ceiling.textureIndex;
		}
		else
		{
			// To do: get ceiling from .INFs without *CEILING (like START.INF). Maybe
			// hardcoding index 1 is enough?
			return 1;
		}
	}();

	// Define the ceiling voxel data.
	const int index = this->voxelGrid.addVoxelData(
		VoxelData::makeCeiling(ceilingIndex));

	// Set all the ceiling voxels.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			this->setVoxel(x, 2, z, index);
		}
	}
}

void LevelData::readLocks(const std::vector<MIFFile::Level::Lock> &locks, int width, int depth)
{
	for (const auto &lock : locks)
	{
		const Int2 lockPosition = VoxelGrid::getTransformedCoordinate(
			Int2(lock.x, lock.y), width, depth);
		this->locks.insert(std::make_pair(
			lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}
}

void LevelData::readTriggers(const std::vector<MIFFile::Level::Trigger> &triggers,
	const INFFile &inf, int width, int depth)
{
	for (const auto &trigger : triggers)
	{
		// Transform the voxel coordinates from the Arena layout to the new layout.
		const Int2 voxel = VoxelGrid::getTransformedCoordinate(
			Int2(trigger.x, trigger.y), width, depth);

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
