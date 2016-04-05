#include <cassert>
#include <map>

#include "Font.h"
#include "TextureName.h"
#include "../Math/Int2.h"

// This gives the logical location of each character, independent of font resolution.
// Multiplication by character width and height will be needed afterwards to get the
// size and location of the actual letter in the font surface.
const auto FontCharacterCells = std::map<unsigned char, Int2>
{
	{ '!', Int2(0, 0) },
	{ '\"', Int2(1, 0) },
	{ '#', Int2(2, 0) },
	{ '$', Int2(3, 0) },
	{ '%', Int2(4, 0) },
	{ '&', Int2(5, 0) },
	{ '\'', Int2(6, 0) },
	{ '(', Int2(7, 0) },
	{ ')', Int2(8, 0) },
	{ '*', Int2(9, 0) },
	{ '+', Int2(10, 0) },
	{ ',', Int2(11, 0) },
	{ '-', Int2(12, 0) },
	{ '.', Int2(13, 0) },
	{ '/', Int2(14, 0) },
	{ '0', Int2(15, 0) },
	{ '1', Int2(16, 0) },
	{ '2', Int2(17, 0) },
	{ '3', Int2(18, 0) },
	{ '4', Int2(19, 0) },
	{ '5', Int2(20, 0) },
	{ '6', Int2(21, 0) },
	{ '7', Int2(22, 0) },
	{ '8', Int2(23, 0) },
	{ '9', Int2(24, 0) },
	{ ':', Int2(25, 0) },
	{ ';', Int2(26, 0) },
	{ '<', Int2(27, 0) },
	{ '=', Int2(28, 0) },
	{ '>', Int2(29, 0) },
	{ '?', Int2(30, 0) },
	{ '@', Int2(31, 0) },
	{ 'A', Int2(0, 1) },
	{ 'B', Int2(1, 1) },
	{ 'C', Int2(2, 1) },
	{ 'D', Int2(3, 1) },
	{ 'E', Int2(4, 1) },
	{ 'F', Int2(5, 1) },
	{ 'G', Int2(6, 1) },
	{ 'H', Int2(7, 1) },
	{ 'I', Int2(8, 1) },
	{ 'J', Int2(9, 1) },
	{ 'K', Int2(10, 1) },
	{ 'L', Int2(11, 1) },
	{ 'M', Int2(12, 1) },
	{ 'N', Int2(13, 1) },
	{ 'O', Int2(14, 1) },
	{ 'P', Int2(15, 1) },
	{ 'Q', Int2(16, 1) },
	{ 'R', Int2(17, 1) },
	{ 'S', Int2(18, 1) },
	{ 'T', Int2(19, 1) },
	{ 'U', Int2(20, 1) },
	{ 'V', Int2(21, 1) },
	{ 'W', Int2(22, 1) },
	{ 'X', Int2(23, 1) },
	{ 'Y', Int2(24, 1) },
	{ 'Z', Int2(25, 1) },
	{ '[', Int2(26, 1) },
	{ '\\', Int2(27, 1) },
	{ ']', Int2(28, 1) },
	{ '^', Int2(29, 1) },
	{ '_', Int2(30, 1) },
	{ 'a', Int2(0, 2) },
	{ 'b', Int2(1, 2) },
	{ 'c', Int2(2, 2) },
	{ 'd', Int2(3, 2) },
	{ 'e', Int2(4, 2) },
	{ 'f', Int2(5, 2) },
	{ 'g', Int2(6, 2) },
	{ 'h', Int2(7, 2) },
	{ 'i', Int2(8, 2) },
	{ 'j', Int2(9, 2) },
	{ 'k', Int2(10, 2) },
	{ 'l', Int2(11, 2) },
	{ 'm', Int2(12, 2) },
	{ 'n', Int2(13, 2) },
	{ 'o', Int2(14, 2) },
	{ 'p', Int2(15, 2) },
	{ 'q', Int2(16, 2) },
	{ 'r', Int2(17, 2) },
	{ 's', Int2(18, 2) },
	{ 't', Int2(19, 2) },
	{ 'u', Int2(20, 2) },
	{ 'v', Int2(21, 2) },
	{ 'w', Int2(22, 2) },
	{ 'x', Int2(23, 2) },
	{ 'y', Int2(24, 2) },
	{ 'z', Int2(25, 2) },
	{ '{', Int2(26, 2) },
	{ '|', Int2(27, 2) },
	{ '}', Int2(28, 2) },
	{ ' ', Int2(30, 2) }
};

// The upper and lower case letters are separated here, and use a simple "islower()"
// or "isupper()" to differentiate which map they draw from. Symbols should be 
// treated like upper case letters. I think.

// Update 3/16/2016:
// When trying to get character spacing right, do Robin Hood with the size and offset;
// take from one and give it to the other.

// 3/18/2016:
// Well the Robin Hood thing assumes mono-space format, which some fonts are not, sadly.
// The *best* solution would be to just have character sizes and offsets for every 
// single character, but that would take a while to program.

// As a better way, just have the letter trimmed of white-space to its right and a 
// little added back to have "adaptive" spacing. This trimming could go in the TextBox
// class.

const auto FontUpperCharacterSizes = std::map<FontName, Int2>
{
	{ FontName::A, Int2(12, 10) },
	{ FontName::Arena, Int2(8, 8) },
	{ FontName::B, Int2(5, 6) },
	{ FontName::C, Int2(14, 14) },
	{ FontName::Char, Int2(6, 6) },
	{ FontName::D, Int2(6, 8) },
	{ FontName::Four, Int2(5, 7) },
	{ FontName::S, Int2(5, 4) },
	{ FontName::Teeny, Int2(5, 6) }
};

const auto FontLowerCharacterSizes = std::map<FontName, Int2>
{
	{ FontName::A, Int2(8, 10) },
	{ FontName::Arena, Int2(8, 8) },
	{ FontName::B, Int2(5, 6) },
	{ FontName::C, Int2(8, 14) },
	{ FontName::Char, Int2(6, 6) },
	{ FontName::D, Int2(6, 8) },
	{ FontName::Four, Int2(5, 7) },
	{ FontName::S, Int2(5, 4) },
	{ FontName::Teeny, Int2(5, 6) }
};

const auto FontUpperCharacterOffsets = std::map<FontName, Int2>
{
	{ FontName::A, Int2(4, 0) },
	{ FontName::Arena, Int2(8, 0) },
	{ FontName::B, Int2(11, 0) },
	{ FontName::C, Int2(2, 0) },
	{ FontName::Char, Int2(10, 2) },
	{ FontName::D, Int2(10, 0) },
	{ FontName::Four, Int2(11, 0) },
	{ FontName::S, Int2(11, 1) },
	{ FontName::Teeny, Int2(11, 2) }
};

const auto FontLowerCharacterOffsets = std::map<FontName, Int2>
{
	{ FontName::A, Int2(8, 1) },
	{ FontName::Arena, Int2(8, 0) },
	{ FontName::B, Int2(11, 0) },
	{ FontName::C, Int2(8, 0) },
	{ FontName::Char, Int2(10, 2) },
	{ FontName::D, Int2(10, 0) },
	{ FontName::Four, Int2(11, 0) },
	{ FontName::S, Int2(11, 1) },
	{ FontName::Teeny, Int2(11, 2) }
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

Int2 Font::getCharacterCell(unsigned char c)
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
