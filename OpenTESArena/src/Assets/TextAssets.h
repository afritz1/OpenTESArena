#ifndef TEXT_ASSETS_H
#define TEXT_ASSETS_H

#include <string>
#include <unordered_map>

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

	// Load TEMPLATE.DAT, grouping blocks of text by their #ID.
	void parseTemplateDat();
public:
	TextAssets();
	~TextAssets();

	// Gets a chunk of A.EXE as a string. Offset and size should be obtained from
	// the Exe strings header file.
	const std::string &getAExeSegment(const std::pair<int, int> &offsetAndSize);

	// Finds the text in TEMPLATE.DAT given a key (i.e., "#0000a").
	const std::string &getTemplateDatText(const std::string &key);
};

#endif
