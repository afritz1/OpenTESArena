#include "BinaryAssetLibrary.h"
#include "../Math/Random.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/vfs/manager.hpp"

ArenaTypes::ClimateType WorldMapTerrain::toClimateType(uint8_t index)
{
	if ((index == WorldMapTerrain::TEMPERATE1) ||
		(index == WorldMapTerrain::TEMPERATE2))
	{
		return ArenaTypes::ClimateType::Temperate;
	}
	else if ((index == WorldMapTerrain::MOUNTAIN1) ||
		(index == WorldMapTerrain::MOUNTAIN2))
	{
		return ArenaTypes::ClimateType::Mountain;
	}
	else if ((index == WorldMapTerrain::DESERT1) ||
		(index == WorldMapTerrain::DESERT2))
	{
		return ArenaTypes::ClimateType::Desert;
	}
	else
	{
		DebugUnhandledReturnMsg(ArenaTypes::ClimateType, std::to_string(static_cast<int>(index)));
	}
}

uint8_t WorldMapTerrain::getNormalizedIndex(uint8_t index)
{
	return index - WorldMapTerrain::SEA;
}

uint8_t WorldMapTerrain::getAt(int x, int y) const
{
	const int index = DebugMakeIndex(this->indices, x + (y * WorldMapTerrain::WIDTH));
	return this->indices[index];
}

uint8_t WorldMapTerrain::getFailSafeAt(int x, int y) const
{
	// Lambda for obtaining a terrain pixel at some XY coordinate.
	auto getTerrainAt = [this](int x, int y)
	{
		const int index = [x, y]()
		{
			const int pixelCount = WorldMapTerrain::WIDTH * WorldMapTerrain::HEIGHT;

			// Move the index 12 pixels left (wrapping around if necessary).
			int i = x + (y * WorldMapTerrain::WIDTH);
			i -= 12;

			if (i < 0)
			{
				i += pixelCount;
			}
			else if (i >= pixelCount)
			{
				i -= pixelCount;
			}

			return i;
		}();

		DebugAssertIndex(this->indices, index);
		return this->indices[index];
	};

	// Try to get the terrain at the requested pixel.
	const uint8_t terrainPixel = getTerrainAt(x, y);

	if (terrainPixel != WorldMapTerrain::SEA)
	{
		// The pixel is a usable terrain.
		return terrainPixel;
	}
	else
	{
		// Fail-safe: check around the requested pixel in a '+' pattern for non-sea pixels.
		for (int dist = 1; dist < 200; dist++)
		{
			const std::array<uint8_t, 4> failSafePixels =
			{
				getTerrainAt(x, y + dist), // Below.
				getTerrainAt(x, y - dist), // Above.
				getTerrainAt(x + dist, y), // Right.
				getTerrainAt(x - dist, y) // Left.
			};

			const auto iter = std::find_if(failSafePixels.begin(), failSafePixels.end(),
				[](uint8_t pixel) { return pixel != WorldMapTerrain::SEA; });

			if (iter != failSafePixels.end())
			{
				return *iter;
			}
		}

		// Give up, returning default temperate terrain.
		return WorldMapTerrain::TEMPERATE1;
	}
}

bool WorldMapTerrain::init(const char *filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	if (stream == nullptr)
	{
		DebugLogError("Could not open \"" + std::string(filename) + "\".");
		return false;
	}

	// Skip the .IMG header.
	stream->seekg(12);
	stream->read(reinterpret_cast<char*>(this->indices.data()), this->indices.size());
	return true;
}

bool BinaryAssetLibrary::initExecutableData(bool floppyVersion)
{
	if (!this->exeData.init(floppyVersion))
	{
		DebugLogError("Could not init .EXE data; is floppy version: " + std::to_string(floppyVersion) + ".");
		return false;
	}

	return true;
}

bool BinaryAssetLibrary::initClasses(const ExeData &exeData)
{
	const char *filename = "CLASSES.DAT";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	// Character class generation members (to be set).
	auto &classes = this->classesDat.classes;
	auto &choices = this->classesDat.choices;

	// The class IDs take up the first 18 bytes.
	for (size_t i = 0; i < std::size(classes); i++)
	{
		const uint8_t value = *(srcPtr + i);

		CharacterClassGenerationClass &genClass = classes[i];
		genClass.id = value & CharacterClassGeneration::ID_MASK;
		genClass.isSpellcaster = (value & CharacterClassGeneration::SPELLCASTER_MASK) != 0;
		genClass.hasCriticalHit = (value & CharacterClassGeneration::CRITICAL_HIT_MASK) != 0;
		genClass.isThief = (value & CharacterClassGeneration::THIEF_MASK) != 0;
	}

	// After the class IDs are 66 groups of "A, B, C" choices. They account for all 
	// the combinations of answers to character questions. When the user is done
	// answering questions, their A/B/C counts map to some index in the Choices array.
	for (size_t i = 0; i < std::size(choices); i++)
	{
		const int choiceSize = 3;
		const size_t offset = std::size(classes) + (choiceSize * i);

		CharacterClassGenerationChoice &choice = choices[i];
		choice.a = *(srcPtr + offset);
		choice.b = *(srcPtr + offset + 1);
		choice.c = *(srcPtr + offset + 2);
	}

	return true;
}

bool BinaryAssetLibrary::initStandardSpells()
{
	// The filename has different casing between the floppy and CD version, so use a
	// case-insensitive open method so it works on case-sensitive systems (i.e., Unix).
	const char *filename = "SPELLSG.65";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().readCaseInsensitive(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());
	ArenaTypes::SpellData::initArray(this->standardSpells, srcPtr);
	return true;
}

bool BinaryAssetLibrary::initWorldMapDefs()
{
	const char *filename = "CITYDATA.65";
	if (!this->cityDataFile.init(filename))
	{
		DebugLogError("Could not init \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
}

bool BinaryAssetLibrary::initWorldMapMasks()
{
	const char *filename = "TAMRIEL.MNU";
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	// Beginning of the mask data.
	constexpr int startOffset = 0x87D5;

	// Each province's mask rectangle is a set of bits packed together with others.
	const std::array<Rect, 10> MaskRects =
	{
		Rect(37, 32, 86, 57),
		Rect(47, 53, 90, 62),
		Rect(113, 29, 88, 53),
		Rect(190, 31, 102, 93),
		Rect(31, 131, 65, 52),
		Rect(100, 118, 61, 55),
		Rect(144, 119, 50, 57),
		Rect(204, 116, 67, 67),
		Rect(103, 72, 131, 84),
		Rect(279, 188, 37, 11) // "Exit" button.
	};

	// Initialize each of the world map masks, moving the offset to the beginning
	// of the next data each loop.
	int offset = 0;
	for (size_t i = 0; i < this->worldMapMasks.size(); i++)
	{
		DebugAssertIndex(MaskRects, i);
		const Rect &rect = MaskRects[i];

		// The number of bytes in the mask rect.
		const int byteCount = WorldMapMask::getAdjustedWidth(rect.getWidth()) * rect.getHeight();

		// Copy the segment of mask bytes to a new vector.
		const uint8_t *maskStart = srcPtr + startOffset + offset;
		const uint8_t *maskEnd = maskStart + byteCount;
		std::vector<uint8_t> maskData(maskStart, maskEnd);

		// Assign the map mask onto the map masks list.
		this->worldMapMasks[i] = WorldMapMask(std::move(maskData), rect);

		// Move to the next mask.
		offset += byteCount;
	}

	return true;
}

bool BinaryAssetLibrary::initWorldMapTerrain()
{
	const char *filename = "TERRAIN.IMG";
	if (!this->worldMapTerrain.init(filename))
	{
		DebugLogWarning("Couldn't init world map terrain \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
}

bool BinaryAssetLibrary::init(bool floppyVersion)
{
	DebugLog("Initializing binary assets.");
	bool success = this->initExecutableData(floppyVersion);
	success &= this->initClasses(this->exeData);
	success &= this->initStandardSpells();
	success &= this->initWorldMapDefs();
	success &= this->initWorldMapMasks();
	success &= this->initWorldMapTerrain();
	return true;
}

const ExeData &BinaryAssetLibrary::getExeData() const
{
	return this->exeData;
}

const CityDataFile &BinaryAssetLibrary::getCityDataFile() const
{
	return this->cityDataFile;
}

const CharacterClassGeneration &BinaryAssetLibrary::getClassGenData() const
{
	return this->classesDat;
}

const ArenaTypes::Spellsg &BinaryAssetLibrary::getStandardSpells() const
{
	return this->standardSpells;
}

const WorldMapMasks &BinaryAssetLibrary::getWorldMapMasks() const
{
	return this->worldMapMasks;
}

const WorldMapTerrain &BinaryAssetLibrary::getWorldMapTerrain() const
{
	return this->worldMapTerrain;
}

const std::string &BinaryAssetLibrary::getRulerTitle(int provinceID,
	ArenaTypes::LocationType locationType, bool isMale, ArenaRandom &random) const
{
	// Get the index into the titles list.
	const int titleIndex = [this, provinceID, locationType, &random, isMale]()
	{
		if (provinceID == ArenaLocationUtils::CENTER_PROVINCE_ID)
		{
			return isMale ? 6 : 13;
		}
		else if (locationType == ArenaTypes::LocationType::CityState)
		{
			return isMale ? 5 : 12;
		}
		else if (locationType == ArenaTypes::LocationType::Village)
		{
			return isMale ? 0 : 7;
		}
		else
		{
			// Random for town.
			const int randVal = (random.next() % 4) + 1;
			return isMale ? randVal : (randVal + 7);
		}
	}();

	const auto &rulerTitles = this->exeData.locations.rulerTitles;
	DebugAssertIndex(rulerTitles, titleIndex);
	return rulerTitles[titleIndex];
}
