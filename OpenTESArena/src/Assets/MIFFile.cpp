#include <algorithm>
#include <unordered_map>

#include "Compression.h"
#include "MIFFile.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

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
	const std::unordered_map<std::string, int(*)(MIFFile::Level&, const uint8_t*)> MIFLevelTags =
	{
		{ Tag_FLAT, MIFFile::Level::loadFLAT },
		{ Tag_FLOR, MIFFile::Level::loadFLOR },
		{ Tag_INFO, MIFFile::Level::loadINFO },
		{ Tag_INNS, MIFFile::Level::loadINNS },
		{ Tag_LOCK, MIFFile::Level::loadLOCK },
		{ Tag_LOOT, MIFFile::Level::loadLOOT },
		{ Tag_MAP1, MIFFile::Level::loadMAP1 },
		{ Tag_MAP2, MIFFile::Level::loadMAP2 },
		{ Tag_NAME, MIFFile::Level::loadNAME },
		{ Tag_NUMF, MIFFile::Level::loadNUMF },
		{ Tag_STOR, MIFFile::Level::loadSTOR },
		{ Tag_TARG, MIFFile::Level::loadTARG },
		{ Tag_TRIG, MIFFile::Level::loadTRIG }
	};
}

MIFFile::Level::Level()
{
	this->numf = 0;
}

const uint8_t MIFFile::DRY_CHASM = 0xC;
const uint8_t MIFFile::WET_CHASM = 0xD;
const uint8_t MIFFile::LAVA_CHASM = 0xE;

MIFFile::MIFFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssertMsg(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	const uint16_t headerSize = Bytes::getLE16(srcData.data() + 4);

	// Get data from the header (after "MHDR"). Constant for all levels. The header 
	// size should be 61.
	ArenaTypes::MIFHeader mifHeader;
	mifHeader.init(srcData.data() + 6);

	// Load start locations from the header. Not all are set (i.e., some are (0, 0)).
	for (size_t i = 0; i < this->startPoints.size(); i++)
	{
		const uint16_t mifX = mifHeader.startX.at(i);
		const uint16_t mifY = mifHeader.startY.at(i);
		
		// Convert the coordinates from .MIF format to voxel format. The remainder
		// of the division is used for positioning within the voxel.
		const double x = static_cast<double>(mifX) / MIFFile::ARENA_UNITS;
		const double y = static_cast<double>(mifY) / MIFFile::ARENA_UNITS;

		this->startPoints.at(i) = Double2(x, y);
	}

	// Start of the level data (at each "LEVL"). Some .MIF files have multiple levels,
	// so this needs to be in a loop.
	int levelOffset = headerSize + 6;

	// The level count is unused since it's inferred by this level loading loop.
	while (levelOffset < srcData.size())
	{
		MIFFile::Level level;

		// Begin loading the level data at the current LEVL, and get the offset
		// to the next LEVL.
		const uint8_t *levelStart = srcData.data() + levelOffset;
		levelOffset += level.load(levelStart);

		// Add to list of levels.
		this->levels.push_back(std::move(level));
	}

	this->width = mifHeader.mapWidth;
	this->depth = mifHeader.mapHeight;
	this->startingLevelIndex = mifHeader.startingLevelIndex;
	this->name = filename;
}

std::string MIFFile::mainQuestDungeonFilename(int dungeonX, int dungeonY, int provinceID)
{
	uint32_t mifID = (dungeonY << 16) + dungeonX + provinceID;
	mifID = static_cast<uint32_t>(-static_cast<int32_t>(Bytes::rol(mifID, 5)));
	const std::string mifName = std::to_string(mifID) + ".MIF";
	return mifName;
}

int MIFFile::getWidth() const
{
	return this->width;
}

int MIFFile::getHeight(int levelIndex) const
{
	return this->levels.at(levelIndex).getHeight();
}

int MIFFile::getDepth() const
{
	return this->depth;
}

int MIFFile::getStartingLevelIndex() const
{
	return this->startingLevelIndex;
}

const std::string &MIFFile::getName() const
{
	return this->name;
}

const std::array<Double2, 4> &MIFFile::getStartPoints() const
{
	return this->startPoints;
}

const std::vector<MIFFile::Level> &MIFFile::getLevels() const
{
	return this->levels;
}

int MIFFile::Level::load(const uint8_t *levelStart)
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

		// Find the function associated with the tag.
		const auto tagIter = MIFLevelTags.find(tag);
		if (tagIter != MIFLevelTags.end())
		{
			// Load the data and move the offset to the beginning of the next tag.
			auto *loadingFn = tagIter->second;
			tagStart += loadingFn(*this, tagStart);
		}
		else
		{
			// This can only be reached if the .MIF file is corrupted.
			DebugCrash("Unrecognized .MIF tag \"" + tag + "\".");
		}
	}

	// Use the updated tag start instead of the level end due to a bug with the LEVL
	// size in WILD.MIF (six bytes short of where it should be, probably due to FLAT
	// tag and size not being accounted for, causing this loader to incorrectly start
	// a second level six bytes from the end of the file).
	return static_cast<int>(std::distance(levelStart, tagStart));
}

int MIFFile::Level::getHeight() const
{
	// If there is MAP2 data, then check through each voxel to find the highest point.
	if (this->map2.size() > 0)
	{
		// @todo: look at MAP2 voxels and determine highest column.
		return 6;
	}
	else
	{
		// Use the default height -- ground, main floor, and ceiling.
		return 3;
	}
}

int MIFFile::Level::loadFLAT(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.flat = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.flat.size(), level.flat.begin());

	return size + 6;
}

int MIFFile::Level::loadFLOR(MIFFile::Level &level, const uint8_t *tagStart)
{
	// Compressed size is in chunks and contains a 2 byte decompressed length after it, which
	// should not be included when determining the end of the compressed range.
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit vector (in little-endian).
	level.flor.resize(decomp.size() / 2);
	std::copy(decomp.begin(), decomp.end(), reinterpret_cast<uint8_t*>(level.flor.data()));

	return compressedSize + 6;
}

int MIFFile::Level::loadINFO(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Using the size might include some unnecessary empty space, so assume it's 
	// null-terminated instead.
	level.info = std::string(reinterpret_cast<const char*>(tagDataStart));

	return size + 6;
}

int MIFFile::Level::loadINNS(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.inns = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.inns.size(), level.inns.begin());

	return size + 6;
}

int MIFFile::Level::loadLOCK(MIFFile::Level &level, const uint8_t *tagStart)
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

int MIFFile::Level::loadLOOT(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown.
	level.loot = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.loot.size(), level.loot.begin());

	return size + 6;
}

int MIFFile::Level::loadMAP1(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit vector (in little-endian).
	level.map1.resize(decomp.size() / 2);
	std::copy(decomp.begin(), decomp.end(), reinterpret_cast<uint8_t*>(level.map1.data()));

	return compressedSize + 6;
}

int MIFFile::Level::loadMAP2(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t compressedSize = Bytes::getLE16(tagStart + 4);
	const uint16_t uncompressedSize = Bytes::getLE16(tagStart + 6);

	// Allocate space for this floor, using 2 bytes per voxel.
	std::vector<uint8_t> decomp(uncompressedSize, 0);

	// Decode the data with type 8 decompression.
	const uint8_t *tagDataStart = tagStart + 8;
	const uint8_t *tagDataEnd = tagStart + 6 + compressedSize;
	Compression::decodeType08(tagDataStart, tagDataEnd, decomp);

	// Write into 16-bit vector (in little-endian).
	level.map2.resize(decomp.size() / 2);
	std::copy(decomp.begin(), decomp.end(), reinterpret_cast<uint8_t*>(level.map2.data()));

	return compressedSize + 6;
}

int MIFFile::Level::loadNAME(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Using the size might include some unnecessary empty space, so assume it's 
	// null-terminated instead.
	level.name = std::string(reinterpret_cast<const char*>(tagDataStart));

	return size + 6;
}

int MIFFile::Level::loadNUMF(MIFFile::Level &level, const uint8_t *tagStart)
{
	// Size should always be 1.
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Just one byte.
	level.numf = *tagDataStart;

	return size + 6;
}

int MIFFile::Level::loadSTOR(MIFFile::Level &level, const uint8_t *tagStart)
{
	const uint16_t size = Bytes::getLE16(tagStart + 4);
	const uint8_t *tagDataStart = tagStart + 6;

	// Currently unknown. Only present in the demo?
	level.stor = std::vector<uint8_t>(size);
	std::copy(tagDataStart, tagDataStart + level.stor.size(), level.stor.begin());

	return size + 6;
}

int MIFFile::Level::loadTARG(MIFFile::Level &level, const uint8_t *tagStart)
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

int MIFFile::Level::loadTRIG(MIFFile::Level &level, const uint8_t *tagStart)
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
