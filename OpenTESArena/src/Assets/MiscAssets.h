#ifndef MISC_ASSETS_H
#define MISC_ASSETS_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "ArenaTypes.h"
#include "CityDataFile.h"
#include "ExeData.h"
#include "WorldMapMask.h"
#include "../Entities/CharacterClass.h"
#include "../Game/CharacterClassGeneration.h"
#include "../Game/CharacterQuestion.h"

// This class stores various miscellaneous data from Arena assets.

// All relevant text files (TEMPLATE.DAT, QUESTION.TXT, etc.) should be read in 
// when this object is created.

class ArenaRandom;

enum class ClimateType;
enum class LocationType;

class MiscAssets
{
public:
	// Each artifact text file (ARTFACT1.DAT, ARTFACT2.DAT) contains conversation strings
	// about artifacts. Supposedly ARTFACT2.DAT is used when the player declines and
	// returns to the individual later.
	// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
	// - Only the first string of barter success is used.
	struct ArtifactTavernText
	{
		std::array<std::string, 3> greetingStrs, barterSuccessStrs,
			offerRefusedStrs, barterFailureStrs, counterOfferStrs;
	};

	// TEMPLATE.DAT stores various strings for in-game text and conversations.
	// Strings #0000 through #0004 have three copies in the file, one for each tileset.
	class TemplateDat
	{
	public:
		struct Entry
		{
			static const int NO_KEY = -1;
			static const char NO_LETTER = -1;

			// Value after the '#' character, excluding any letter at the end of the line.
			int key;

			// Strings #0000-#0004 and #0014 have a letter to further divide each series
			// by the current season + weather. -1 if unused.
			char letter;

			// Ampersand-separated strings.
			std::vector<std::string> values;
		};
	private:
		// One vector for each tileset. Most entries are independent of the current
		// season/weather.
		std::vector<std::vector<Entry>> entryLists;
	public:
		const Entry &getEntry(int key) const;
		const Entry &getEntry(int key, char letter) const;
		const Entry &getTilesetEntry(int tileset, int key, char letter) const;

		void init();
	};

	// Each trade text file (EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, TAVERN.DAT) is an array
	// of 75 null-terminated strings. Each function array wraps conversation behaviors
	// (introduction, price agreement, etc.). Each personality array wraps personalities.
	// Each random array contains three strings for each personality.
	// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
	struct TradeText
	{
		typedef std::array<std::string, 3> RandomArray;
		typedef std::array<RandomArray, 5> PersonalityArray;
		typedef std::array<PersonalityArray, 5> FunctionArray;
		FunctionArray equipment, magesGuild, selling, tavern;
	};

	class WorldMapTerrain
	{
	private:
		static const int WIDTH = 320;
		static const int HEIGHT = 200;

		static const uint8_t TEMPERATE1 = 254;
		static const uint8_t TEMPERATE2 = 251;
		static const uint8_t MOUNTAIN1 = 249;
		static const uint8_t MOUNTAIN2 = 250;
		static const uint8_t DESERT1 = 253;
		static const uint8_t DESERT2 = 252;
		static const uint8_t SEA = 248;

		// 320x200 palette indices.
		std::array<uint8_t, WorldMapTerrain::WIDTH * WorldMapTerrain::HEIGHT> indices;
	public:
		// Converts a terrain index to a climate type. The given index must be for a land pixel.
		static ClimateType toClimateType(uint8_t index);

		// Converts a terrain index to a normalized index (such that sea = 0).
		static uint8_t getNormalizedIndex(uint8_t index);

		// Gets the terrain at the given XY coordinate without any correction.
		uint8_t getAt(int x, int y) const;

		// Gets the terrain at the given XY coordinate (also accounts for the 12 pixel
		// error and does a fail-safe search for sea pixels).
		uint8_t getFailSafeAt(int x, int y) const;

		void init();
	};
private:
	ExeData exeData; // Either floppy version or CD version (depends on ArenaPath).
	TemplateDat templateDat;
	std::vector<CharacterQuestion> questionTxt;
	CharacterClassGeneration classesDat;
	std::vector<CharacterClass> classDefinitions;
	std::vector<std::pair<std::string, std::string>> dungeonTxt;
	std::array<ArtifactTavernText, 16> artifactTavernText1, artifactTavernText2;
	TradeText tradeText;
	std::vector<std::vector<std::string>> nameChunks;
	CityDataFile cityDataFile;
	ArenaTypes::Spellsg standardSpells; // From SPELLSG.65.
	std::array<std::string, 43> spellMakerDescriptions; // From SPELLMKR.TXT.
	std::array<WorldMapMask, 10> worldMapMasks;
	WorldMapTerrain worldMapTerrain;

	// Loads the executable associated with the current Arena data path (either A.EXE
	// for the floppy version or ACD.EXE for the CD version).
	void parseExecutableData(bool floppyVersion);

	// Load QUESTION.TXT and separate each question by its number.
	void parseQuestionTxt();

	// Load CLASSES.DAT and also read class data from the executable.
	void parseClasses(const ExeData &exeData);

	// Load DUNGEON.TXT and pair each dungeon name with its description.
	void parseDungeonTxt();

	// Loads ARTFACT1.DAT and ARTFACT2.DAT.
	void parseArtifactText();

	// Loads EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, and TAVERN.DAT.
	void parseTradeText();

	// Loads NAMECHNK.DAT into a jagged list of name chunks.
	void parseNameChunks();

	// Loads SPELLSG.65.
	void parseStandardSpells();

	// Loads SPELLMKR.TXT.
	void parseSpellMakerDescriptions();

	// Reads the mask data from TAMRIEL.MNU.
	void parseWorldMapMasks();
public:
	MiscAssets();

	// Gets the ExeData object. There may be slight differences between A.EXE and ACD.EXE,
	// but only one will be available at a time for the lifetime of the program (dependent
	// on the Arena path in the options).
	const ExeData &getExeData() const;

	// Gets the TEMPLATE.DAT object for accessing strings by their ID and optional letter.
	const TemplateDat &getTemplateDat() const;

	// Returns all of the questions in QUESTION.TXT.
	const std::vector<CharacterQuestion> &getQuestionTxtQuestions() const;

	const CharacterClassGeneration &getClassGenData() const;
	const std::vector<CharacterClass> &getClassDefinitions() const;

	// Returns all of the main quest dungeon names paired with their description. 
	// These are just the dungeons with a unique icon on the world map, not the 
	// lesser dungeons.
	const std::vector<std::pair<std::string, std::string>> &getDungeonTxtDungeons() const;

	// Gets the artifact text used in tavern conversations.
	const std::array<ArtifactTavernText, 16> &getArtifactTavernText1() const;
	const std::array<ArtifactTavernText, 16> &getArtifactTavernText2() const;

	// Gets the trade text object for trade conversations.
	const MiscAssets::TradeText &getTradeText() const;

	// Gets the ruler title associated with the given parameters.
	const std::string &getRulerTitle(int provinceID, LocationType locationType,
		bool isMale, ArenaRandom &random) const;

	// Creates a random NPC name from the given parameters.
	std::string generateNpcName(int raceID, bool isMale, ArenaRandom &random) const;

	// Gets the data object for world map locations.
	const CityDataFile &getCityDataFile() const;

	// Gets the spells list for spell and effect definitions.
	const ArenaTypes::Spellsg &getStandardSpells() const;

	// Gets the list of spell maker description strings.
	const std::array<std::string, 43> &getSpellMakerDescriptions() const;

	// Gets the mask rectangles used for registering clicks on the world map. There are
	// ten entries -- the first nine are provinces and the last is the "Exit" button.
	const std::array<WorldMapMask, 10> &getWorldMapMasks() const;

	// Gets the world map terrain used with climate and travel calculations.
	const WorldMapTerrain &getWorldMapTerrain() const;

	void init(bool floppyVersion);
};

#endif
