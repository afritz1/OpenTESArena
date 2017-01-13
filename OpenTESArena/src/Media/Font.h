#ifndef FONT_H
#define FONT_H

#include <string>
#include <vector>

// Redesigned for use with Arena assets.

enum class FontName;

struct SDL_Surface;

class Font
{
private:
	// ASCII character-indexed surfaces, where space (ASCII 32) is index 0.
	std::vector<SDL_Surface*> characters;
	FontName fontName;
	int characterHeight;
public:
	// Constructs a group of characters using an image of bits from a font file.
	Font(FontName fontName);
	Font(Font &&font);
	~Font();

	Font &operator=(const Font &font) = delete;
	Font &operator=(Font &&font) = delete;

	// Gets the filename for a given font name.
	static const std::string &fromName(FontName fontName);

	// Gets the height in pixels for all characters in the font.
	int getCharacterHeight() const;

	// Gets the name of the selected font.
	FontName getFontName() const;

	// Gets the surface for a given character.
	SDL_Surface *getSurface(char c) const;
};

#endif
