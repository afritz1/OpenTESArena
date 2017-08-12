#include "SDL.h"

#include "RichTextString.h"

#include "../Media/Font.h"
#include "../Media/FontManager.h"
#include "../Utilities/String.h"

RichTextString::RichTextString(const std::string &text, FontName fontName,
	const Color &color, TextAlignment alignment, int lineSpacing, FontManager &fontManager)
	: text(text), fontName(fontName), color(color), alignment(alignment),
	lineSpacing(lineSpacing)
{
	// The dimensions are expensive to calculate, so it's done at construction time.
	this->dimensions = [&text, fontName, lineSpacing, &fontManager]()
	{
		// Get the font data associated with the font name.
		const Font &font = fontManager.getFont(fontName);

		// Split the text into separate lines. If the text is empty, then just add a 
		// space so there doesn't need to be any "zero-character" special cases.
		const std::vector<std::string> textLines = RichTextString::splitLines(text);

		// Retrieve a pointer for each character's surface in each line.
		std::vector<std::vector<const SDL_Surface*>> lineSurfaces;

		for (const auto &textLine : textLines)
		{
			lineSurfaces.push_back(RichTextString::getTextSurfaces(textLine, font));
		}

		// Get the width of each line in pixels (for determining the longest line).
		const std::vector<int> lineWidths = [&lineSurfaces]()
		{
			std::vector<int> widths;

			for (const auto &charSurfaces : lineSurfaces)
			{
				// Start a new count on the line widths.
				widths.push_back(0);

				// Get the combined widths for the current line's surfaces.
				for (const auto *surface : charSurfaces)
				{
					widths.back() += surface->w;
				}
			}

			return widths;
		}();

		// Get the width in pixels for the final SDL texture.
		const int textureWidth = [&lineWidths]()
		{
			// Get the width in pixels of the longest line.
			int maxWidth = 0;
			for (const int lineWidth : lineWidths)
			{
				if (lineWidth > maxWidth)
				{
					maxWidth = lineWidth;
				}
			}
			return maxWidth;
		}();

		// Get the height in pixels for all characters in a font.
		const int characterHeight = font.getCharacterHeight();

		// Get the height in pixels for the final SDL texture. Also include line spacing.
		const int lineCount = static_cast<int>(textLines.size());
		const int textureHeight = (characterHeight * lineCount) +
			(lineSpacing * (lineCount - 1));

		return Int2(textureWidth, textureHeight);
	}();
}

RichTextString::RichTextString(const std::string &text, FontName fontName,
	const Color &color, TextAlignment alignment, FontManager &fontManager)
	: RichTextString(text, fontName, color, alignment, 0, fontManager) { }

RichTextString::~RichTextString()
{

}

const std::string &RichTextString::getText() const
{
	return this->text;
}

FontName RichTextString::getFontName() const
{
	return this->fontName;
}

const Color &RichTextString::getColor() const
{
	return this->color;
}

const Int2 &RichTextString::getDimensions() const
{
	return this->dimensions;
}

TextAlignment RichTextString::getAlignment() const
{
	return this->alignment;
}

int RichTextString::getLineSpacing() const
{
	return this->lineSpacing;
}

std::vector<std::string> RichTextString::splitLines(const std::string &text)
{
	// If the text is empty, then just add a space, so there don't need to be
	// any "zero-character" special cases.
	return String::split((text.size() > 0) ? text : std::string(" "), '\n');
}

std::vector<const SDL_Surface*> RichTextString::getTextSurfaces(
	const std::string &line, const Font &font)
{
	std::vector<const SDL_Surface*> lineSurfaces;

	// Add a pointer to each surface associated with each character.
	for (const char c : line)
	{
		const SDL_Surface *charSurface = font.getSurface(c);
		lineSurfaces.push_back(charSurface);
	}

	return lineSurfaces;
}
