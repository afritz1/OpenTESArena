#ifndef IMG_FILE_H
#define IMG_FILE_H

#include <string>

// An IMG file can have one of a few formats; either with a header that determines
// properties, or without a header (either raw or a wall). Some IMGs also have a
// built-in palette, which they may or may not use eventually.

class IMGFileType;

class IMGFile
{
public:
	IMGFile(const std::string &filename);
	~IMGFile();
};

#endif
