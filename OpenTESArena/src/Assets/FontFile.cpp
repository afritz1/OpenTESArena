#include <algorithm>
#include <array>
#include <unordered_map>

#include "FontFile.h"

#include "../Media/Color.h"
#include "../Media/Font.h"
#include "../Media/FontName.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

namespace
{
	// Number of columns of whitespace a space is.
	const std::unordered_map<std::string, int> FontSpaceWidths =
	{
		{ "FONT_A.DAT", 5 },
		{ "ARENAFNT.DAT", 3 },
		{ "FONT_B.DAT", 3 },
		{ "FONT_C.DAT", 4 },
		{ "CHARFNT.DAT", 3 },
		{ "FONT_D.DAT", 3 },
		{ "FONT4.DAT", 3 },
		{ "FONT_S.DAT", 3 },
		{ "TEENYFNT.DAT", 2 }
	};

	struct FontElement
	{
		uint32_t width, height;
		std::array<uint16_t, 16> lines;

		FontElement()
		{
			this->width = 0;
			this->height = 0;
			std::fill(this->lines.begin(), this->lines.end(), 0);
		}
	};
}

FontFile::FontFile(const std::string &filename)
	: characters()
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "Font File", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// The character height is in the first byte.
	const uint8_t charHeight = srcData.at(0);
	const uint8_t *counts = srcData.data();
	const uint16_t *lines = reinterpret_cast<uint16_t*>(srcData.data() + 95);

	std::array<FontElement, 96> symbols;
	std::fill(symbols.begin(), symbols.end(), FontElement());

	// Set the space character as some arbitrary size.
	FontElement &space = symbols.at(0);
	space.width = FontSpaceWidths.at(filename);
	space.height = charHeight;

	// Adapted from WinArena "ParseBSA.cpp".
	for (size_t i = 1; i < symbols.size(); ++i)
	{
		FontElement &element = symbols.at(i);
		element.height = charHeight;

		// For each line of pixel data, check how many pixels are needed to draw it.
		// Update the max width with the current maximum as it is found.
		uint32_t maxWidth = 0;
		for (uint32_t lineNum = 0; lineNum < element.height; ++lineNum)
		{
			uint16_t &line = element.lines.at(lineNum);
			line = *lines;
			lines++;

			uint16_t mask = 0x8000;
			for (uint32_t c = 0; c < 16; ++c)
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
	
	// Now that the symbols table is filled with character bits, turn it into a list
	// of characters paired with a width and pixel data.
	this->characterHeight = charHeight;
	this->characters.resize(symbols.size());

	// Colors for setting pixels.
	const uint32_t black = Color(0, 0, 0, 0).toARGB();
	const uint32_t white = Color(255, 255, 255, 255).toARGB();

	// Adapted from WinArena "Raster.cpp".
	for (size_t i = 0; i < symbols.size(); ++i)
	{
		// Use white for pixels and black for background.
		FontElement &element = symbols.at(i);

		auto &pair = this->characters.at(i);
		pair = std::make_pair(element.width, std::unique_ptr<uint32_t[]>(
			new uint32_t[element.width * element.height]));

		uint32_t *pixels = pair.second.get();
		for (uint32_t cy = 0; cy < element.height; ++cy)
		{
			uint16_t mask = 0x8000;
			uint16_t bits = element.lines.at(cy);

			for (uint32_t cx = 0; cx < element.width; ++cx)
			{
				const int index = cx + (cy * element.width);

				// Color the pixel white if the character's bit is set there.
				pixels[index] = ((bits & mask) != 0) ? white : black;

				mask >>= 1;
			}
		}
	}
}

FontFile::~FontFile()
{

}

int FontFile::getWidth(char c) const
{
	Debug::check((c >= 32) && (c <= 127), "Font File", "Character value \"" +
		std::to_string(c) + "\" out of range (must be ASCII 32-127).");

	// Space (ASCII 32) is at index 0.
	return this->characters.at(c - 32).first;
}

int FontFile::getHeight() const
{
	return this->characterHeight;
}

uint32_t *FontFile::getPixels(char c) const
{
	Debug::check((c >= 32) && (c <= 127), "Font File", "Character value \"" +
		std::to_string(c) + "\" out of range (must be ASCII 32-127).");

	// Space (ASCII 32) is at index 0.
	return this->characters.at(c - 32).second.get();
}
