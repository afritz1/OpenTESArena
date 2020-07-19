#ifndef PALETTE_UTILS_H
#define PALETTE_UTILS_H

#include <string>

namespace PaletteUtils
{
	// Returns whether the given palette name is "built-in" or not.
	bool isBuiltIn(const std::string &filename);
}

#endif
