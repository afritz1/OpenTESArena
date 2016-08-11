#ifndef FONT_FILE_H
#define FONT_FILE_H

#include <string>

// This class will load .DAT files containing font information, and return them 
// each in the form of an image. Each cell in the image would hold a particular 
// character, symbol, or empty space.

// This will eventually replace the "Font" class in the Media folder.

class FontFile
{
public:
	FontFile(const std::string &filename);
	~FontFile();
};

#endif
