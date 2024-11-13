#ifndef BINARY_ASSET_LIBRARY_H
#define BINARY_ASSET_LIBRARY_H

#include <array>
#include <cstdint>

#include "ArenaTypes.h"
#include "CityDataFile.h"
#include "ExeData.h"
#include "WorldMapMask.h"
#include "../Game/CharacterClassGeneration.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/Singleton.h"

class ArenaRandom;

// Contains assets that are generally not human-readable.
class BinaryAssetLibrary : public Singleton<BinaryAssetLibrary>
{
public:
	class WorldMapTerrain
	{
	private:
		static constexpr int WIDTH = 320;
		static constexpr int HEIGHT = 200;

		static constexpr uint8_t TEMPERATE1 = 254;
		static constexpr uint8_t TEMPERATE2 = 251;
		static constexpr uint8_t MOUNTAIN1 = 249;
		static constexpr uint8_t MOUNTAIN2 = 250;
		static constexpr uint8_t DESERT1 = 253;
		static constexpr uint8_t DESERT2 = 252;
		static constexpr uint8_t SEA = 248;

		// 320x200 palette indices.
		std::array<uint8_t, WorldMapTerrain::WIDTH * WorldMapTerrain::HEIGHT> indices;
	public:
		// Converts a terrain index to a climate type. The given index must be for a land pixel.
		static ArenaTypes::ClimateType toClimateType(uint8_t index);

		// Converts a terrain index to a normalized index (such that sea = 0).
		static uint8_t getNormalizedIndex(uint8_t index);

		// Gets the terrain at the given XY coordinate without any correction.
		uint8_t getAt(int x, int y) const;

		// Gets the terrain at the given XY coordinate (also accounts for the 12 pixel
		// error and does a fail-safe search for sea pixels).
		uint8_t getFailSafeAt(int x, int y) const;

		bool init(const char *filename);
	};

	using WorldMapMasks = std::array<WorldMapMask, 10>;
private:
	ExeData exeData; // Either floppy version or CD version (depends on ArenaPath).
	CityDataFile cityDataFile;
	CharacterClassGeneration classesDat;
	ArenaTypes::Spellsg standardSpells; // From SPELLSG.65.
	WorldMapMasks worldMapMasks;
	WorldMapTerrain worldMapTerrain;

	// Loads the executable associated with the current Arena data path (either A.EXE
	// for the floppy version or ACD.EXE for the CD version).
	bool initExecutableData(bool floppyVersion);

	// Load CLASSES.DAT and also read class data from the executable.
	bool initClasses(const ExeData &exeData);

	// Loads SPELLSG.65.
	bool initStandardSpells();

	// Loads world map definitions from CITYDATA.65.
	bool initWorldMapDefs();

	// Reads the mask data from TAMRIEL.MNU.
	bool initWorldMapMasks();

	// Loads world map terrain.
	bool initWorldMapTerrain();
public:
	bool init(bool floppyVersion);

	// Gets the ExeData object. There may be slight differences between A.EXE and ACD.EXE,
	// but only one will be available at a time for the lifetime of the program (dependent
	// on the Arena path in the options).
	const ExeData &getExeData() const;

	// Gets the original game's world map location data.
	const CityDataFile &getCityDataFile() const;

	const CharacterClassGeneration &getClassGenData() const;

	// Gets the spells list for spell and effect definitions.
	const ArenaTypes::Spellsg &getStandardSpells() const;

	// Gets the mask rectangles used for registering clicks on the world map. There are
	// ten entries -- the first nine are provinces and the last is the "Exit" button.
	const WorldMapMasks &getWorldMapMasks() const;

	// Gets the world map terrain used with climate and travel calculations.
	const WorldMapTerrain &getWorldMapTerrain() const;

	// Gets the ruler title associated with the given parameters.
	const std::string &getRulerTitle(int provinceID, ArenaTypes::LocationType locationType,
		bool isMale, ArenaRandom &random) const;
};

#endif
