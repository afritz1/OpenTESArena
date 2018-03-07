#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <unordered_map>

#include "Font.h"

// This class manages access for each font object. This should be stored in the 
// game state with the other managers.

enum class FontName;

class FontManager
{
private:
	std::unordered_map<FontName, Font> fonts;
public:
	// Gets a font object using one of the Arena font assets.
	const Font &getFont(FontName fontName);
};

#endif
