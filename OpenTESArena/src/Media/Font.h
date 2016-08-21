#ifndef FONT_H
#define FONT_H

#include <cstdint>

// Made some changes to this class during the text box redesign. No more character
// widths and heights for each font. All letters have their right side whitespace
// trimmed until the number of whitespace columns for padding is reached.

class Int2;

enum class FontName;
enum class TextureName;

class Font
{
private:
	FontName fontName;
public:
	Font(FontName fontName);
	~Font();

	static Int2 getCellPosition(uint8_t c);

	FontName getFontName() const;
	TextureName getFontTextureName() const;

	// Get the width and height of each of a font's cells in pixels. This tells how
	// much to copy at each cell position.
	Int2 getCellDimensions() const;

	// The number of columns of whitespace to have on the right side of each letter.
	int32_t getRightPadding() const;

	// The number of columns of whitespace a space is.
	int32_t getSpaceWidth() const;
};

#endif
