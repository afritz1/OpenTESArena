#include <algorithm>
#include <tuple>
#include <unordered_map>

#include "Compression.h"
#include "MIFFile.h"
#include "MIFUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

namespace
{
	const std::string Tag_FLAT = "FLAT";
	const std::string Tag_FLOR = "FLOR";
	const std::string Tag_INFO = "INFO";
	const std::string Tag_INNS = "INNS";
	const std::string Tag_LEVL = "LEVL";
	const std::string Tag_LOCK = "LOCK";
	const std::string Tag_LOOT = "LOOT";
	const std::string Tag_MAP1 = "MAP1";
	const std::string Tag_MAP2 = "MAP2";
	const std::string Tag_NAME = "NAME";
	const std::string Tag_NUMF = "NUMF";
	const std::string Tag_STOR = "STOR";
	const std::string Tag_TARG = "TARG";
	const std::string Tag_TRIG = "TRIG";

	// Mappings of .MIF level tags to functions for decoding data, excluding "LEVL".
	using LevelTagFuncTable = std::unordered_map<std::string, int(*)(MIFLevel&, const uint8_t*)>;
	using LevelTagWithDimsFuncTable = std::unordered_map<std::string, int(*)(MIFLevel&, const uint8_t*, WEInt, SNInt)>;

	const LevelTagFuncTable MIFLevelTags =
	{
		{ Tag_FLAT, MIFLevel::loadFLAT },
		{ Tag_INFO, MIFLevel::loadINFO },
		{ Tag_INNS, MIFLevel::loadINNS },
		{ Tag_LOCK, MIFLevel::loadLOCK },
		{ Tag_LOOT, MIFLevel::loadLOOT },
		{ Tag_NAME, MIFLevel::loadNAME },
		{ Tag_NUMF, MIFLevel::loadNUMF },
		{ Tag_STOR, MIFLevel::loadSTOR },
		{ Tag_TARG, MIFLevel::loadTARG },
		{ Tag_TRIG, MIFLevel::loadTRIG }
	};

	const LevelTagWithDimsFuncTable MIFLevelTagsWithDims =
	{
		{ Tag_FLOR, MIFLevel::loadFLOR },
		{ Tag_MAP1, MIFLevel::loadMAP1 },
		{ Tag_MAP2, MIFLevel::loadMAP2 }
	};
}

MIFLevel::MIFLevel()
{
	this->numf = 0;
}

int MIFLevel::load(const uint8_t *levelStart, WEInt levelWidth, SNInt levelDepth)
{
	// Get the size of the level data.
	const uint16_t levelSize = Bytes::getLE16(levelStart + 4);

	// Move the tag pointer while there are tags to read in the current level.
	const uint8_t *tagStart = levelStart + 6;
	const uint8_t *levelEnd = tagStart + levelSize;
	while (tagStart < levelEnd)
	{
		// Check what the four letter tag is (FLOR, MAP1, etc., never LEVL).
		const std::string tag(tagStart, tagStart + 4);

		// Find the function associated with the tag; it may or may not need level dimensions.
		const auto tagIter = MIFLevelTags.find(tag);
		if (tagIter != MIFLevelTags.end())
		{
			// Load the data and move the offset to the beginning of the next tag.
			auto *loadingFunc = tagIter->second;
			tagStart += loadingFunc(*this, tagStart);
		}
		else
		{
			// See if the tag is associated with a function that needs level parameters.
			const auto tagDimsIter = MIFLevelTagsWithDims.find(tag);
			if (tagDimsIter != MIFLevelTagsWithDims.end())
			{
				// Load the data and move the offset to the beginning of the next tag.
				auto *loadingFuncWithDims = tagDimsIter->second;
				tagStart += loadingFuncWithDims(*this, tagStart, levelWidth, levelDepth);
			}
			else
			{
				// This can only be reached if the .MIF file is corrupted. Can't continue
				// loading because the next tag is unknown.
				DebugLogWarning("Unrecognized .MIF tag \"" + tag + "\".");
				break;
			}
		}
	}

	// Use the updated tag start instead of the level end due to a bug with the LEVL
	// size in WILD.MIF (six bytes short of where it should be, probably due to FLAT
	// tag and size not being accounted for, causing this loader to incorrectly start
	// a second level six bytes from the end of the file).
	return static_cast<int>(std::distance(levelStart, tagStart));
}

int MIFLevel::loadFLAT(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.flat = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.flat.size(), level.flat.begin());

	return size + 6;
}

int MIFLevel::loadFLOR(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth)
{
	// Compressed size is in chunks and contains a 2 byte decompressed length after it, which
	// should not be included when determining the end of the compressed range.
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel. Note: this seems to be more
	// space than the level can fit.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit map voxels in little-endian.
	level.flor.init(levelWidth, levelDepth);
	for (SNInt z = 0; z < levelDepth; z++)
	{
		for (WEInt x = 0; x < levelWidth; x++)
		{
			const int srcIndex = (x + (z * levelWidth)) * sizeof(ArenaTypes::VoxelID);
			DebugAssertIndex(decomp, srcIndex);
			const ArenaTypes::VoxelID srcValue = Bytes::getLE16(decomp.data() + srcIndex);
			level.flor.set(x, z, srcValue);
		}
	}

	return compressedSize + 6;
}

int MIFLevel::loadMAP1(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth)
{
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel. Note: this seems to be more
	// space than the level can fit.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit map voxels in little-endian.
	level.map1.init(levelWidth, levelDepth);
	for (SNInt z = 0; z < levelDepth; z++)
	{
		for (WEInt x = 0; x < levelWidth; x++)
		{
			const int srcIndex = (x + (z * levelWidth)) * sizeof(ArenaTypes::VoxelID);
			DebugAssertIndex(decomp, srcIndex);
			const ArenaTypes::VoxelID srcValue = Bytes::getLE16(decomp.data() + srcIndex);
			level.map1.set(x, z, srcValue);
		}
	}

	return compressedSize + 6;
}

int MIFLevel::loadMAP2(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth)
{
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel. Note: this seems to be more
	// space than the level can fit.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit map voxels in little-endian.
	level.map2.init(levelWidth, levelDepth);
	for (SNInt z = 0; z < levelDepth; z++)
	{
		for (WEInt x = 0; x < levelWidth; x++)
		{
			const int srcIndex = (x + (z * levelWidth)) * sizeof(ArenaTypes::VoxelID);
			DebugAssertIndex(decomp, srcIndex);
			const ArenaTypes::VoxelID srcValue = Bytes::getLE16(decomp.data() + srcIndex);
			level.map2.set(x, z, srcValue);
		}
	}

	return compressedSize + 6;
}

int MIFLevel::loadINFO(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Using the size might include some unnecessary empty space, so assume it's 
	// null-terminated instead.
	level.info = std::string(reinterpret_cast<const char*>(tagDataStart));

	return size + 6;
}

int MIFLevel::loadINNS(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.inns = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.inns.size(), level.inns.begin());

	return size + 6;
}

int MIFLevel::loadLOCK(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);

	// Each lock record is 3 bytes.
	const int lockCount = size / 3;
	level.lock = std::vector<ArenaTypes::MIFLock>(lockCount);

	const uint8_t *tagDataStart = tagStart + 6;
	for (int i = 0; i < lockCount; i++)
	{
		const int offset = i * 3;

		ArenaTypes::MIFLock &lock = level.lock.at(i);
		lock.init(tagDataStart + offset);
	}

	return size + 6;
}

int MIFLevel::loadLOOT(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.loot = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.loot.size(), level.loot.begin());

	return size + 6;
}

int MIFLevel::loadNAME(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Using the size might include some unnecessary empty space, so assume it's 
	// null-terminated instead.
	level.name = std::string(reinterpret_cast<const char*>(tagDataStart));

	return size + 6;
}

int MIFLevel::loadNUMF(MIFLevel &level, const uint8_t *tagStart)
{
	// Size should always be 1.
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Just one byte.
	level.numf = *tagDataStart;

	return size + 6;
}

int MIFLevel::loadSTOR(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown. Only present in the demo?
	level.stor = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.stor.size(), level.stor.begin());

	return size + 6;
}

int MIFLevel::loadTARG(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);

	// Each target record is 2 bytes.
	const int targetCount = size / 2;
	level.targ = std::vector<ArenaTypes::MIFTarget>(targetCount);

	const uint8_t *tagDataStart = tagStart + 6;
	for (int i = 0; i < targetCount; i++)
	{
		const int offset = i * 2;

		ArenaTypes::MIFTarget &target = level.targ.at(i);
		target.init(tagDataStart + offset);
	}

	return size + 6;
}

int MIFLevel::loadTRIG(MIFLevel &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);

	// Each trigger record is 4 bytes.
	const int triggerCount = size / 4;
	level.trig = std::vector<ArenaTypes::MIFTrigger>(triggerCount);

	const uint8_t *tagDataStart = tagStart + 6;
	for (int i = 0; i < triggerCount; i++)
	{
		const int offset = i * 4;

		ArenaTypes::MIFTrigger &trigger = level.trig.at(i);
		trigger.init(tagDataStart + offset);
	}

	return size + 6;
}

const std::string &MIFLevel::getName() const
{
	return this->name;
}

const std::string &MIFLevel::getInfo() const
{
	return this->info;
}

int MIFLevel::getNumf() const
{
	return this->numf;
}

BufferView2D<const ArenaTypes::VoxelID> MIFLevel::getFLOR() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(this->flor);
}

BufferView2D<const ArenaTypes::VoxelID> MIFLevel::getMAP1() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(this->map1);
}

BufferView2D<const ArenaTypes::VoxelID> MIFLevel::getMAP2() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(this->map2);
}

BufferView<const uint8_t> MIFLevel::getFLAT() const
{
	return BufferView<const uint8_t>(this->flat);
}

BufferView<const uint8_t> MIFLevel::getINNS() const
{
	return BufferView<const uint8_t>(this->inns);
}

BufferView<const uint8_t> MIFLevel::getLOOT() const
{
	return BufferView<const uint8_t>(this->loot);
}

BufferView<const uint8_t> MIFLevel::getSTOR() const
{
	return BufferView<const uint8_t>(this->stor);
}

BufferView<const ArenaTypes::MIFTarget> MIFLevel::getTARG() const
{
	return BufferView<const ArenaTypes::MIFTarget>(this->targ);
}

BufferView<const ArenaTypes::MIFLock> MIFLevel::getLOCK() const
{
	return BufferView<const ArenaTypes::MIFLock>(this->lock);
}

BufferView<const ArenaTypes::MIFTrigger> MIFLevel::getTRIG() const
{
	return BufferView<const ArenaTypes::MIFTrigger>(this->trig);
}

bool MIFFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	this->filename = filename;
	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());
	const uint16_t headerSize = Bytes::getLE16(srcPtr + 4);

	// Get data from the header (after "MHDR"). Constant for all levels. The header 
	// size should be 61.
	ArenaTypes::MIFHeader mifHeader;
	mifHeader.init(srcPtr + 6);

	// Load start locations from the header. Not all are set (i.e., some are (0, 0)).
	for (size_t i = 0; i < this->startPoints.size(); i++)
	{
		static_assert(std::tuple_size<decltype(mifHeader.startX)>::value ==
			std::tuple_size<decltype(this->startPoints)>::value);
		static_assert(std::tuple_size<decltype(mifHeader.startY)>::value ==
			std::tuple_size<decltype(this->startPoints)>::value);

		const uint16_t mifX = mifHeader.startX[i];
		const uint16_t mifY = mifHeader.startY[i];
		this->startPoints[i] = OriginalInt2(mifX, mifY);
	}

	// Start of the level data (at each "LEVL"). Some .MIF files have multiple levels,
	// so this needs to be in a loop.
	int levelOffset = headerSize + 6;

	// The level count is unused since it's inferred by this level loading loop.
	while (levelOffset < src.getCount())
	{
		MIFLevel level;

		// Begin loading the level data at the current LEVL, and get the offset
		// to the next LEVL.
		const uint8_t *levelStart = srcPtr + levelOffset;
		levelOffset += level.load(levelStart, mifHeader.mapWidth, mifHeader.mapHeight);

		// Add to list of levels.
		this->levels.emplace_back(std::move(level));
	}

	this->width = mifHeader.mapWidth;
	this->depth = mifHeader.mapHeight;
	this->startingLevelIndex = mifHeader.startingLevelIndex;
	return true;
}

const std::string &MIFFile::getFilename() const
{
	return this->filename;
}

WEInt MIFFile::getWidth() const
{
	return this->width;
}

SNInt MIFFile::getDepth() const
{
	return this->depth;
}

int MIFFile::getStartingLevelIndex() const
{
	return this->startingLevelIndex;
}

int MIFFile::getStartPointCount() const
{
	return static_cast<int>(this->startPoints.size());
}

const OriginalInt2 &MIFFile::getStartPoint(int index) const
{
	DebugAssertIndex(this->startPoints, index);
	return this->startPoints[index];
}

int MIFFile::getLevelCount() const
{
	return static_cast<int>(this->levels.size());
}

const MIFLevel &MIFFile::getLevel(int index) const
{
	DebugAssertIndex(this->levels, index);
	return this->levels[index];
}
