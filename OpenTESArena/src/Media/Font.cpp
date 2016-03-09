#include <cassert>
#include <map>

#include "Font.h"
#include "TextureName.h"
#include "../Math/Point.h"

// This gives the logical location of each character, independent of font resolution.
// Multiplication by character width and height will be needed afterwards to get the
// size and location of the actual letter in the font surface.
const auto FontCharacterCells = std::map<unsigned char, Point>
{
	{ '!', Point(0, 0) },
	{ '\"', Point(1, 0) },
	{ '#', Point(2, 0) },
	{ '$', Point(3, 0) },
	{ '%', Point(4, 0) },
	{ '&', Point(5, 0) },
	{ '\'', Point(6, 0) },
	{ '(', Point(7, 0) },
	{ ')', Point(8, 0) },
	{ '*', Point(9, 0) },
	{ '+', Point(10, 0) },
	{ ',', Point(11, 0) },
	{ '-', Point(12, 0) },
	{ '.', Point(13, 0) },
	{ '/', Point(14, 0) },
	{ '0', Point(15, 0) },
	{ '1', Point(16, 0) },
	{ '2', Point(17, 0) },
	{ '3', Point(18, 0) },
	{ '4', Point(19, 0) },
	{ '5', Point(20, 0) },
	{ '6', Point(21, 0) },
	{ '7', Point(22, 0) },
	{ '8', Point(23, 0) },
	{ '9', Point(24, 0) },
	{ ':', Point(25, 0) },
	{ ';', Point(26, 0) },
	{ '<', Point(27, 0) },
	{ '=', Point(28, 0) },
	{ '>', Point(29, 0) },
	{ '?', Point(30, 0) },
	{ '@', Point(31, 0) },
	{ 'A', Point(0, 1) },
	{ 'B', Point(1, 1) },
	{ 'C', Point(2, 1) },
	{ 'D', Point(3, 1) },
	{ 'E', Point(4, 1) },
	{ 'F', Point(5, 1) },
	{ 'G', Point(6, 1) },
	{ 'H', Point(7, 1) },
	{ 'I', Point(8, 1) },
	{ 'J', Point(9, 1) },
	{ 'K', Point(10, 1) },
	{ 'L', Point(11, 1) },
	{ 'M', Point(12, 1) },
	{ 'N', Point(13, 1) },
	{ 'O', Point(14, 1) },
	{ 'P', Point(15, 1) },
	{ 'Q', Point(16, 1) },
	{ 'R', Point(17, 1) },
	{ 'S', Point(18, 1) },
	{ 'T', Point(19, 1) },
	{ 'U', Point(20, 1) },
	{ 'V', Point(21, 1) },
	{ 'W', Point(22, 1) },
	{ 'X', Point(23, 1) },
	{ 'Y', Point(24, 1) },
	{ 'Z', Point(25, 1) },
	{ '[', Point(26, 1) },
	{ '\\', Point(27, 1) },
	{ ']', Point(28, 1) },
	{ '^', Point(29, 1) },
	{ '_', Point(30, 1) },
	{ 'a', Point(0, 2) },
	{ 'b', Point(1, 2) },
	{ 'c', Point(2, 2) },
	{ 'd', Point(3, 2) },
	{ 'e', Point(4, 2) },
	{ 'f', Point(5, 2) },
	{ 'g', Point(6, 2) },
	{ 'h', Point(7, 2) },
	{ 'i', Point(8, 2) },
	{ 'j', Point(9, 2) },
	{ 'k', Point(10, 2) },
	{ 'l', Point(11, 2) },
	{ 'm', Point(12, 2) },
	{ 'n', Point(13, 2) },
	{ 'o', Point(14, 2) },
	{ 'p', Point(15, 2) },
	{ 'q', Point(16, 2) },
	{ 'r', Point(17, 2) },
	{ 's', Point(18, 2) },
	{ 't', Point(19, 2) },
	{ 'u', Point(20, 2) },
	{ 'v', Point(21, 2) },
	{ 'w', Point(22, 2) },
	{ 'x', Point(23, 2) },
	{ 'y', Point(24, 2) },
	{ 'z', Point(25, 2) },
	{ '{', Point(26, 2) },
	{ '|', Point(27, 2) },
	{ '}', Point(28, 2) },
	{ ' ', Point(30, 2) }
};

// The upper and lower case letters are separated here, and use a simple "islower()"
// or "isupper()" to differentiate which map they draw from. Symbols should be 
// treated like upper case letters. I think.
const auto FontUpperCharacterSizes = std::map<FontName, Point>
{
	{ FontName::A, Point(14, 10) },
	{ FontName::Arena, Point(8, 8) },
	{ FontName::B, Point(5, 6) },
	{ FontName::C, Point(14, 14) },
	{ FontName::Char, Point(6, 6) },
	{ FontName::D, Point(6, 8) },
	{ FontName::Four, Point(5, 7) },
	{ FontName::S, Point(5, 4) },
	{ FontName::Teeny, Point(5, 6) }
};

const auto FontLowerCharacterSizes = std::map<FontName, Point>
{
	{ FontName::A, Point(14, 10) },
	{ FontName::Arena, Point(8, 8) },
	{ FontName::B, Point(5, 6) },
	{ FontName::C, Point(14, 14) },
	{ FontName::Char, Point(6, 6) },
	{ FontName::D, Point(6, 8) },
	{ FontName::Four, Point(5, 7) },
	{ FontName::S, Point(5, 4) },
	{ FontName::Teeny, Point(5, 6) }
};

const auto FontUpperCharacterOffsets = std::map<FontName, Point>
{
	{ FontName::A, Point(2, 0) },
	{ FontName::Arena, Point(8, 0) },
	{ FontName::B, Point(11, 0) },
	{ FontName::C, Point(2, 0) },
	{ FontName::Char, Point(10, 2) },
	{ FontName::D, Point(10, 0) },
	{ FontName::Four, Point(11, 0) },
	{ FontName::S, Point(11, 1) },
	{ FontName::Teeny, Point(11, 2) }
};

const auto FontLowerCharacterOffsets = std::map<FontName, Point>
{
	{ FontName::A, Point(2, 0) },
	{ FontName::Arena, Point(8, 0) },
	{ FontName::B, Point(11, 0) },
	{ FontName::C, Point(2, 0) },
	{ FontName::Char, Point(10, 2) },
	{ FontName::D, Point(10, 0) },
	{ FontName::Four, Point(11, 0) },
	{ FontName::S, Point(11, 1) },
	{ FontName::Teeny, Point(11, 2) }
};

const auto FontTextureNames = std::map<FontName, TextureName>
{
	{ FontName::A, TextureName::FontA },
	{ FontName::Arena, TextureName::FontArena },
	{ FontName::B, TextureName::FontB },
	{ FontName::C, TextureName::FontC },
	{ FontName::Char, TextureName::FontChar },
	{ FontName::D, TextureName::FontD },
	{ FontName::Four, TextureName::FontFour },
	{ FontName::S, TextureName::FontS },
	{ FontName::Teeny, TextureName::FontTeeny }
};

Font::Font(FontName fontName)
{
	this->fontName = fontName;
}

Font::~Font()
{

}

Point Font::getCharacterCell(unsigned char c)
{
	auto cell = FontCharacterCells.at(c);
	return cell;
}

const FontName &Font::getFontName() const
{
	return this->fontName;
}

TextureName Font::getFontTextureName() const
{
	auto textureName = FontTextureNames.at(this->getFontName());
	return textureName;
}

int Font::getUpperCharacterWidth() const
{
	int width = FontUpperCharacterSizes.at(this->getFontName()).getX();
	assert(width >= 0);
	return width;
}

int Font::getUpperCharacterHeight() const
{
	int height = FontUpperCharacterSizes.at(this->getFontName()).getY();
	assert(height >= 0);
	return height;
}

int Font::getLowerCharacterWidth() const
{
	int width = FontLowerCharacterSizes.at(this->getFontName()).getX();
	assert(width >= 0);
	return width;
}

int Font::getLowerCharacterHeight() const
{
	int height = FontLowerCharacterSizes.at(this->getFontName()).getY();
	assert(height >= 0);
	return height;
}

int Font::getUpperCharacterOffsetWidth() const
{
	int width = FontUpperCharacterOffsets.at(this->getFontName()).getX();
	assert(width >= 0);
	return width;
}

int Font::getUpperCharacterOffsetHeight() const
{
	int height = FontUpperCharacterOffsets.at(this->getFontName()).getY();
	assert(height >= 0);
	return height;
}

int Font::getLowerCharacterOffsetWidth() const
{
	int width = FontLowerCharacterOffsets.at(this->getFontName()).getX();
	assert(width >= 0);
	return width;
}

int Font::getLowerCharacterOffsetHeight() const
{
	int height = FontLowerCharacterOffsets.at(this->getFontName()).getY();
	assert(height >= 0);
	return height;
}
