#ifndef MISC_ASSETS_H
#define MISC_ASSETS_H

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "CityDataFile.h"
#include "WorldMapMask.h"
#include "../Entities/CharacterClass.h"
#include "../Game/CharacterClassGeneration.h"
#include "../Game/CharacterQuestion.h"

// This class stores various miscellaneous data from Arena assets.

// All relevant text files (TEMPLATE.DAT, QUESTION.TXT, etc.) should be read in 
// when this object is created.

class ExeStrings;

class MiscAssets
{
public:
	// Values for interior and exterior wall heights. In wilderness cells, the values in
	// box1 and box2 are multiplied by 192/256. In interiors, they are also scaled by the
	// ceiling2 value (second value in *CEILING lines; default=128?).
	struct WallHeightTables
	{
		std::array<uint16_t, 8> box1a, box1b, box1c;
		std::array<uint16_t, 16> box2a, box2b;
		// Ignore "source" array, a copy of previous 56 words.
		std::array<uint16_t, 8> box3a, box3b;
		std::array<uint16_t, 16> box4;
	};
private:
	static const std::string AExeKeyValuesMapPath;

	std::string aExe;
	std::unique_ptr<ExeStrings> aExeStrings;
	std::unordered_map<std::string, std::string> templateDat;
	std::vector<CharacterQuestion> questionTxt;
	CharacterClassGeneration classesDat;
	std::vector<CharacterClass> classDefinitions;
	std::vector<std::pair<std::string, std::string>> dungeonTxt;
	CityDataFile cityDataFile;
	WallHeightTables wallHeightTables;
	std::array<WorldMapMask, 10> worldMapMasks;

	// Load TEMPLATE.DAT, grouping blocks of text by their #ID.
	void parseTemplateDat();

	// Load QUESTION.TXT and separate each question by its number.
	void parseQuestionTxt();

	// Load CLASSES.DAT and also read class data from A.EXE. The dataSegmentOffset is
	// the start position of the .data segment in the executable (used with allowed
	// shields and weapons).
	void parseClasses(const std::string &exeText, const ExeStrings &exeStrings, 
		int dataSegmentOffset);

	// Load DUNGEON.TXT and pair each dungeon name with its description.
	void parseDungeonTxt();

	// Reads the wall height data from A.EXE.
	void parseWallHeightTables(const std::string &exeText);

	// Reads the mask data from TAMRIEL.MNU.
	void parseWorldMapMasks();
public:
	MiscAssets();
	~MiscAssets();

	// Gets the ExeStrings object for obtaining floppy disk executable strings with.
	const ExeStrings &getAExeStrings() const;

	// Finds the text in TEMPLATE.DAT given a key (i.e., "#0000a").
	const std::string &getTemplateDatText(const std::string &key);

	// Returns all of the questions in QUESTION.TXT.
	const std::vector<CharacterQuestion> &getQuestionTxtQuestions() const;

	const CharacterClassGeneration &getClassGenData() const;
	const std::vector<CharacterClass> &getClassDefinitions() const;

	// Returns all of the main quest dungeon names paired with their description. 
	// These are just the dungeons with a unique icon on the world map, not the 
	// lesser dungeons.
	const std::vector<std::pair<std::string, std::string>> &getDungeonTxtDungeons() const;

	// Gets the data object for world map locations.
	const CityDataFile &getCityDataFile() const;

	// Gets the wall height tables for determining how tall walls are.
	const WallHeightTables &getWallHeightTables() const;

	// Gets the mask rectangles used for registering clicks on the world map. There are
	// ten entries -- the first nine are provinces and the last is the "Exit" button.
	const std::array<WorldMapMask, 10> &getWorldMapMasks() const;
};

#endif
