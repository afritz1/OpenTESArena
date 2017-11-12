#ifndef COL_FILE_H
#define COL_FILE_H

#include <string>

#include "../Media/Palette.h"

// A COL file (assuming it means "color" file) has the colors for a palette.

// This is a static class because the only interface needed is for reading 
// the palette once.

class COLFile
{
private:
	COLFile() = delete;
	~COLFile() = delete;
public:
	// Writes the data of a COL file into the given palette reference.
	static void toPalette(const std::string &filename, Palette &dstPalette);
};

#endif
