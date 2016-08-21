#ifndef TEXT_ASSETS_H
#define TEXT_ASSETS_H

#include <string>
#include <unordered_map>

// This class stores various plain text (human readable) data from Arena assets.

// All relevant text files (TEMPLATE.DAT, QUESTION.TXT, etc.) should be read in 
// when this object is created.

class TextAssets
{
private:
	std::unordered_map<std::string, std::string> templateDat;

	// Load TEMPLATE.DAT, grouping blocks of text by their #ID.
	void parseTemplateDat();
public:
	TextAssets();
	~TextAssets();

	// Finds the text in TEMPLATE.DAT given a key (i.e., "#0000a").
	const std::string &getTemplateDatText(const std::string &key);
};

#endif
