#ifndef FONT_H
#define FONT_H

#include "FontName.h"

class Int2;

enum class TextureName;

class Font
{
private:
	FontName fontName;
public:
	Font(FontName fontName);
	~Font();

	static Int2 getCharacterCell(unsigned char c);

	const FontName &getFontName() const;
	TextureName getFontTextureName() const;
	int getUpperCharacterWidth() const;
	int getUpperCharacterHeight() const;
	int getLowerCharacterWidth() const;
	int getLowerCharacterHeight() const;
	int getUpperCharacterOffsetWidth() const;
	int getUpperCharacterOffsetHeight() const;
	int getLowerCharacterOffsetWidth() const;
	int getLowerCharacterOffsetHeight() const;
};

#endif