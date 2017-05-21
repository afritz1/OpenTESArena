#ifndef TEXT_ASSETS_H
#define TEXT_ASSETS_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../Game/CharacterQuestion.h"

// This class stores various plain text (human readable) data from Arena assets.

// All relevant text files (TEMPLATE.DAT, QUESTION.TXT, etc.) should be read in 
// when this object is created.

namespace std
{
	// Hash function for std::pair<int, int>.
	template <>
	struct hash<std::pair<int, int>>
	{
		size_t operator()(const std::pair<int, int> &p) const
		{
			// Multiplied "second" by a prime number to make it a tiny bit stronger.
			return static_cast<size_t>(p.first ^ (p.second * 149));
		}
	};
}

class TextAssets
{
private:
	std::string aExe;
	std::unordered_map<std::pair<int, int>, std::string> aExeSegments;
	std::unordered_map<std::string, std::string> templateDat;
	std::vector<CharacterQuestion> questionTxt;
	std::vector<std::pair<std::string, std::string>> dungeonTxt;

	// Load TEMPLATE.DAT, grouping blocks of text by their #ID.
	void parseTemplateDat();

	// Load QUESTION.TXT and separate each question by its number.
	void parseQuestionTxt();

	// Load DUNGEON.TXT and pair each dungeon name with its description.
	void parseDungeonTxt();
public:
	TextAssets();
	~TextAssets();

	// Gets a chunk of A.EXE as a string. Offset and size should be obtained from
	// the Exe strings header file.
	const std::string &getAExeSegment(const std::pair<int, int> &offsetAndSize);

	// Finds the text in TEMPLATE.DAT given a key (i.e., "#0000a").
	const std::string &getTemplateDatText(const std::string &key);

	// Returns all of the questions in QUESTION.TXT.
	const std::vector<CharacterQuestion> &getQuestionTxtQuestions() const;

	// Returns all of the main quest dungeon names paired with their description. 
	// These are just the dungeons with a unique icon on the world map, not the 
	// lesser dungeons.
	const std::vector<std::pair<std::string, std::string>> &getDungeonTxtDungeons() const;
};

#endif
