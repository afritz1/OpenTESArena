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

const auto FontCellDimensions = std::map<FontName, Int2>
{
	{ FontName::A, Int2(16, 11) },
	{ FontName::Arena, Int2(16, 9) },
	{ FontName::B, Int2(16, 6) },
	{ FontName::C, Int2(16, 14) },
	{ FontName::Char, Int2(16, 8) },
	{ FontName::D, Int2(16, 7) },
	{ FontName::Four, Int2(16, 7) },
	{ FontName::S, Int2(16, 5) },
	{ FontName::Teeny, Int2(16, 8) }
};

// Number of columns of whitespace on the right of each letter.
const auto FontRightPaddings = std::map<FontName, int>
{
	{ FontName::A, 1 },
	{ FontName::Arena, 1 },
	{ FontName::B, 1 },
	{ FontName::C, 1 },
	{ FontName::Char, 1 },
	{ FontName::D, 1 },
	{ FontName::Four, 1 },
	{ FontName::S, 1 },
	{ FontName::Teeny, 1 }
};

// Number of columns of whitespace a space is.
const auto FontSpaceWidths = std::map<FontName, int>
{
	{ FontName::A, 5 },
	{ FontName::Arena, 3 },
	{ FontName::B, 3 },
	{ FontName::C, 5 },
	{ FontName::Char, 3 },
	{ FontName::D, 3 },
	{ FontName::Four, 3 },
	{ FontName::S, 3 },
	{ FontName::Teeny, 2 }
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

Int2 Font::getCellPosition(unsigned char c)
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

Int2 Font::getCellDimensions() const
{
	auto dimensions = FontCellDimensions.at(this->getFontName());
	return dimensions;
}

int Font::getRightPadding() const
{
	int padding = FontRightPaddings.at(this->getFontName());
	return padding;
}

int Font::getSpaceWidth() const
{
	int width = FontSpaceWidths.at(this->getFontName());
	return width;
}
