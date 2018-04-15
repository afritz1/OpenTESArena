#ifndef COL_FILE_H
#define COL_FILE_H

#include <string>

#include "../Media/Palette.h"

// A COL file (assuming it means "color" file) has the colors for a palette.

class COLFile
{
private:
	Palette palette;
public:
	COLFile(const std::string &filename);

	const Palette &getPalette() const;
};

#endif
