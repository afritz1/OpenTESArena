#include <algorithm>
#include <cassert>
#include <iostream>

#include "SDL.h"

#include "TextBox.h"

#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/Font.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"

// The Surface constructor is given (1, 1) for width and height because the text box
// dimensions are calculated in the TextBox constructor, and a surface cannot have
// width and height of zero. They are placeholder values, essentially.
TextBox::TextBox(int x, int y, const Color &textColor, const std::string &text,
	FontName fontName, TextureManager &textureManager)
	: Surface(x, y, 1, 1)
{
	this->fontName = fontName;

	// Split "text" into separate lines of text.
	auto textLines = this->textToLines(text);

	// Get combined surfaces for each text line. That is, each line is one surface.
	std::vector<std::unique_ptr<Surface>> lineSurfaces;
	for (const auto &textLine : textLines)
	{
		auto letterSurfaces = this->lineToSurfaces(textLine, textureManager);

		// Don't try to combine empty lines.
		if (letterSurfaces.size() > 0)
		{
			lineSurfaces.push_back(this->combineSurfaces(letterSurfaces));
		}
	}

	// Calculate the proper dimensions of the new text box surface.
	int width = this->getMaxWidth(lineSurfaces);
	int height = this->getNewLineHeight(textureManager) *
		static_cast<int>(lineSurfaces.size());

	// Replace the old SDL surface. It was just a placeholder until now.
	SDL_FreeSurface(this->surface);
	this->surface = SDL_CreateRGBSurface(0, width, height, Surface::DEFAULT_BPP, 0, 0, 0, 0);
	this->optimize(textureManager.getFormat());

	// Make the parent surface transparent.
	this->fill(Color::Magenta);
	this->setTransparentColor(Color::Magenta);

	// Copy each line surface to the parent surface.
	int yOffset = 0;
	for (const auto &lineSurface : lineSurfaces)
	{
		lineSurface->blit(*this, Int2(0, yOffset));
		yOffset += lineSurface->getHeight();
	}

	// Set the text to the text color.
	auto *pixels = static_cast<unsigned int*>(this->surface->pixels);
	for (int i = 0; i < (this->getWidth() * this->getHeight()); ++i)
	{
		auto pixelColor = Color::fromRGB(pixels[i]);
		if (pixelColor != Color::Magenta)
		{
			pixels[i] = textColor.toRGB();
		}
	}
}

// This constructor is identical to the other one, except this one aligns the
// text to center over the given point. (Maybe this can call the other constructor,
// and just translate "this->point" by the center?)
TextBox::TextBox(const Int2 &center, const Color &textColor, const std::string &text,
	FontName fontName, TextureManager &textureManager)
	: Surface(0, 0, 1, 1)
{
	this->fontName = fontName;

	// Split "text" into separate lines of text.
	auto textLines = this->textToLines(text);

	// Get combined surfaces for each text line. That is, each line is one surface.
	std::vector<std::unique_ptr<Surface>> lineSurfaces;
	for (const auto &textLine : textLines)
	{
		auto letterSurfaces = this->lineToSurfaces(textLine, textureManager);

		// Don't try to combine empty lines.
		if (letterSurfaces.size() > 0)
		{
			lineSurfaces.push_back(this->combineSurfaces(letterSurfaces));
		}
	}

	// Calculate the proper dimensions of the new text box surface.
	int width = this->getMaxWidth(lineSurfaces);
	int height = this->getNewLineHeight(textureManager) *
		static_cast<int>(lineSurfaces.size());

	// Replace the old SDL surface. It was just a placeholder until now.
	SDL_FreeSurface(this->surface);
	this->surface = SDL_CreateRGBSurface(0, width, height, Surface::DEFAULT_BPP, 0, 0, 0, 0);
	this->optimize(textureManager.getFormat());
	this->setX(center.getX() - (width / 2));
	this->setY(center.getY() - (height / 2));

	// Make the parent surface transparent.
	this->fill(Color::Magenta);
	this->setTransparentColor(Color::Magenta);

	// Copy each line surface to the parent surface.
	int yOffset = 0;
	for (const auto &lineSurface : lineSurfaces)
	{
		int xOffset = (width / 2) - (lineSurface->getWidth() / 2);
		lineSurface->blit(*this, Int2(xOffset, yOffset));
		yOffset += lineSurface->getHeight();
	}

	// Set the text to the text color.
	auto *pixels = static_cast<unsigned int*>(this->surface->pixels);
	for (int i = 0; i < (this->getWidth() * this->getHeight()); ++i)
	{
		auto pixelColor = Color::fromRGB(pixels[i]);
		if (pixelColor != Color::Magenta)
		{
			pixels[i] = textColor.toRGB();
		}
	}
}

TextBox::~TextBox()
{

}

int TextBox::getNewLineHeight(TextureManager &textureManager) const
{
	Font font(this->getFontName());
	const auto &surface = textureManager.getSurface(
		TextureFile::fromName(font.getFontTextureName()));
	int height = surface.getHeight() / 3;
	return height;
}

int TextBox::getMaxWidth(const std::vector<std::unique_ptr<Surface>> &surfaces)
{
	int maxWidth = 0;
	for (const auto &surface : surfaces)
	{
		maxWidth = std::max(maxWidth, surface->getWidth());
	}

	return maxWidth;
}

std::vector<std::string> TextBox::textToLines(const std::string &text) const
{
	std::vector<std::string> lines;

	// Add one empty string to start.
	lines.push_back(std::string());

	const char newLine = '\n';
	int textLineIndex = 0;
	for (const auto &c : text)
	{
		if (c == newLine)
		{
			// Add a new line.
			lines.push_back(std::string());
			textLineIndex++;
		}
		else
		{
			// Add the character onto the current line.
			// std::string has push_back! Yay! Intuition + 1.
			lines.at(textLineIndex).push_back(c);
		}
	}

	return lines;
}

std::unique_ptr<Surface> TextBox::combineSurfaces(
	const std::vector<std::unique_ptr<Surface>> &letterSurfaces)
{
	int totalWidth = 0;

	// Get the sum of all the surfaces' widths.
	for (const auto &surface : letterSurfaces)
	{
		totalWidth += surface->getWidth();
	}

	int totalHeight = letterSurfaces.at(0)->getHeight();

	// Make the big surface.
	auto combinedSurface = std::unique_ptr<Surface>(new Surface(
		totalWidth, totalHeight));

	// Copy each little surface to the big one.
	int offset = 0;
	for (const auto &surface : letterSurfaces)
	{
		surface->blit(*combinedSurface.get(), Int2(offset, 0));
		offset += surface->getWidth();
	}

	return combinedSurface;
}

std::vector<std::unique_ptr<Surface>> TextBox::lineToSurfaces(const std::string &line,
	TextureManager &textureManager) const
{
	// This method gets each letter surface, trims it to the required right padding,
	// and puts it into a vector, thus representing a line of blit-able text.
	std::vector<std::unique_ptr<Surface>> surfaces;

	// For empty strings (new lines), just put a single space character.
	if (line.size() == 0)
	{
		surfaces.push_back(this->getTrimmedLetter(' ', textureManager));
	}
	else
	{
		for (const auto &c : line)
		{
			surfaces.push_back(this->getTrimmedLetter(c, textureManager));
		}
	}

	return surfaces;
}

std::unique_ptr<Surface> TextBox::getTrimmedLetter(unsigned char c,
	TextureManager &textureManager) const
{
	// Get the font and its associated texture.
	const Font font(this->fontName);
	const auto &fontTexture = textureManager.getSurface(
		TextureFile::fromName(font.getFontTextureName()));
	const auto *fontPixels = static_cast<unsigned int*>(fontTexture.getSurface()->pixels);

	// Get the font properties. "Space" is a special case because it is completely
	// whitespace (i.e., can't be trimmed), so it needs an arbitrary width.
	const int letterHeight = this->getNewLineHeight(textureManager);
	const int rightPadding = font.getRightPadding();
	const int spaceWidth = font.getSpaceWidth();
	const auto cellDimensions = font.getCellDimensions();
	const char space = ' ';

	// If the character is not a space, figure out what its trimmed dimensions are.
	if (c != space)
	{
		// Logical position of the character in the texture (like a 2D table).
		const auto cell = Font::getCellPosition(c);

		// The top left corner of the cell in pixels.
		const Int2 corner(cell.getX() * cellDimensions.getX(),
			cell.getY() * cellDimensions.getY());

		// Calculate the letter's trimmed width by walking columns from right to left
		// in the font texture until a text pixel is hit.
		int letterWidth = cellDimensions.getX();
		for (bool textPixelHit = false; (!textPixelHit) && (letterWidth > 0); --letterWidth)
		{
			for (int j = 0; j < letterHeight; ++j)
			{
				int x = corner.getX() + (letterWidth - 1);
				int y = corner.getY() + j;
				int pixelIndex = x + (y * fontTexture.getWidth());

				auto pixelColor = Color::fromRGB(fontPixels[pixelIndex]);
				if (pixelColor != Color::Magenta)
				{
					textPixelHit = true;
					break;
				}
			}
		}

		// Now add the right padding onto the letter width, after hitting a text pixel.
		// The "+ 1" is for recovering from the column with the text pixel hit.
		letterWidth += rightPadding + 1;

		// If the letter width (before padding) is zero, that means no text pixels were found,
		// so the character in the font texture must be empty. Use a space then.
		if (letterWidth == rightPadding)
		{
			auto surface = std::unique_ptr<Surface>(new Surface(spaceWidth, letterHeight));
			surface->fill(Color::Magenta);
			return surface;
		}
		else
		{
			// Make the letter surface and copy the character into it from the font texture.
			auto letterSurface = std::unique_ptr<Surface>(new Surface(letterWidth, letterHeight));
			fontTexture.blit(*letterSurface.get(), Int2(),
				Rect(corner.getX(), corner.getY(), letterWidth, letterHeight));
			return letterSurface;
		}
	}
	else
	{
		// The character is a space, so return the whitespace surface instead.
		auto surface = std::unique_ptr<Surface>(new Surface(spaceWidth, letterHeight));
		surface->fill(Color::Magenta);
		return surface;
	}
}

FontName TextBox::getFontName() const
{
	return this->fontName;
}
