#include <algorithm>
#include <array>
#include <string>

#include "FontFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

namespace
{
	struct FontElement
	{
		std::array<uint16_t, 16> lines;
		uint32_t width, height;

		FontElement()
		{
			this->lines.fill(0);
			this->width = 0;
			this->height = 0;
		}
	};
}

bool FontFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	// The character height is in the first byte.
	const uint8_t charHeight = srcPtr[0];
	const uint8_t *counts = srcPtr;
	const uint16_t *lines = reinterpret_cast<const uint16_t*>(counts + 95);

	std::array<FontElement, 96> symbols;
	symbols.fill(FontElement());

	// Start at index 1 since the width of a space (index 0) depends on the exclamation mark.
	// Adapted from WinArena "ParseBSA.cpp".
	for (size_t i = 1; i < symbols.size(); i++)
	{
		FontElement &element = symbols[i];
		element.height = charHeight;

		// For each line of pixel data, check how many pixels are needed to draw it.
		// Update the max width with the current maximum as it is found.
		uint32_t maxWidth = 0;
		for (uint32_t lineNum = 0; lineNum < element.height; lineNum++)
		{
			uint16_t &line = element.lines.at(lineNum);
			line = *lines;
			lines++;

			uint16_t mask = 0x8000;
			for (uint32_t c = 0; c < 16; c++)
			{
				if (((line & mask) != 0) && (maxWidth < (c + 1)))
				{
					maxWidth = c + 1;
				}

				mask >>= 1;
			}
		}

		// Put one column of whitespace on the rightmost side for spacing between
		// characters when drawn.
		maxWidth += 1;
		element.width = maxWidth;
	}

	// Assign the exclamation mark's dimensions to space (' ').
	FontElement &space = symbols.front();
	space.width = symbols.at(1).width;
	space.height = charHeight;
	
	// Now that the symbols table is filled with character bits, turn it into a list
	// of characters paired with a width and pixel data.
	this->characterHeight = charHeight;
	this->characters.resize(symbols.size());

	// Adapted from WinArena "Raster.cpp".
	for (size_t i = 0; i < symbols.size(); i++)
	{
		// Use white for pixels and transparent for background.
		FontElement &element = symbols[i];

		Buffer2D<Pixel> &character = this->characters.at(i);
		character.init(element.width, element.height);

		Pixel *charPixels = character.get();
		for (uint32_t cy = 0; cy < element.height; cy++)
		{
			uint16_t mask = 0x8000;
			uint16_t bits = element.lines.at(cy);

			for (uint32_t cx = 0; cx < element.width; cx++)
			{
				const int index = cx + (cy * element.width);

				// Color the pixel 'true' if the character's bit is set there.
				charPixels[index] = (bits & mask) != 0;

				mask >>= 1;
			}
		}
	}

	return true;
}

bool FontFile::tryGetCharacterIndex(char c, int *outIndex)
{
	// Space (ASCII 32) is at index 0.
	if ((c < 32) || (c > 127))
	{
		DebugLogWarning("Character value \"" + std::string(1, c) +
			"\" out of range (must be ASCII 32-127).");
		return false;
	}

	*outIndex = c - 32;
	return true;
}

bool FontFile::tryGetChar(int index, char *outChar)
{
	// Space (ASCII 32) is at index 0.
	if ((index < 0) || (index > 95))
	{
		DebugLogWarning("Character index \"" + std::to_string(index) +
			"\" out of range (must be 0-95).");
		return false;
	}

	*outChar = index + 32;
	return true;
}

int FontFile::getCharacterCount() const
{
	return static_cast<int>(this->characters.size());
}

int FontFile::getWidth(int index) const
{
	DebugAssertIndex(this->characters, index);
	return this->characters[index].getWidth();
}

int FontFile::getHeight() const
{
	return this->characterHeight;
}

const FontFile::Pixel *FontFile::getPixels(int index) const
{
	DebugAssertIndex(this->characters, index);
	return this->characters[index].get();
}
