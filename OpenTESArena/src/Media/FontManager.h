#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <map>

// This class manages access for each font object. This should be stored in the 
// game state with the other managers.

class Font;

enum class FontName;

class FontManager
{
private:
	std::map<FontName, Font> fonts;
public:
	FontManager();
	~FontManager();

	// Gets a font object using one of the Arena font assets.
	const Font &getFont(FontName fontName);
};

#endif
