#pragma once

#include "../Utilities/Palette.h"

// A COL file (assuming it means "color" file) has the colors for a palette.
class COLFile
{
private:
	Palette palette;
public:
	bool init(const char *filename);

	const Palette &getPalette() const;
};
