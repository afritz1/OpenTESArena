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

	// Gets the mask rectangles used for registering clicks on the world map. There are
	// ten entries -- the first nine are provinces and the last is the "Exit" button.
	const std::array<WorldMapMask, 10> &getWorldMapMasks() const;
};

#endif
