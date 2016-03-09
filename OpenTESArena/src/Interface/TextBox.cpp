#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "TextBox.h"
#include "../Math/Point.h"
#include "../Math/Rectangle.h"
#include "../Media/Color.h"
#include "../Media/Font.h"
#include "../Media/TextureManager.h"

TextBox::TextBox(int x, int y, const Color &color, const std::string &text,
	FontName fontName, TextureManager &textureManager)
	: Surface(x, y, 1, 1)
{
	this->fontName = fontName;

	// Split "text" into lines of text.
	this->textLines = [&text]()
	{
		auto temp = std::vector<std::string>();

		// Add one initial line to start.
		temp.push_back(std::string());

		const char newLine = '\n';
		int textLineIndex = 0;
		for (auto &c : text)
		{
			if (c != newLine)
			{
				// std::string has push_back! Yay! Intuition + 1.
				temp.at(textLineIndex).push_back(c);
			}
			else
			{
				temp.push_back(std::string());
				textLineIndex++;
			}
		}

		return temp;
	}();

	// Calculate the proper dimensions of the text box.
	// No need for a "FontSurface" type... just do the heavy lifting in this class.
	auto font = Font(fontName);
	int upperCharWidth = font.getUpperCharacterWidth();
	int upperCharHeight = font.getUpperCharacterHeight();
	int lowerCharWidth = font.getLowerCharacterWidth();
	int lowerCharHeight = font.getLowerCharacterHeight();

	const int newWidth = [](const std::vector<std::string> &lines,
		int upperCharWidth, int lowerCharWidth)
	{
		// Find longest line (in pixels) out of all lines.
		int longestLength = 0;
		for (auto &line : lines)
		{
			// Sum of all character lengths in the current line.
			int linePixels = 0;
			for (auto &c : line)
			{
				linePixels += islower(c) ? lowerCharWidth : upperCharWidth;
			}

			// Check against the current longest line.
			if (linePixels > longestLength)
			{
				longestLength = linePixels;
			}
		}

		// In pixels.
		return longestLength;
	}(this->textLines, upperCharWidth, lowerCharWidth);

	const int newHeight = static_cast<int>(this->textLines.size()) * upperCharHeight;

	// Resize the surface with the proper dimensions for fitting all the text.
	SDL_FreeSurface(this->surface);
	this->surface = [](int width, int height, int colorBits)
	{
		return SDL_CreateRGBSurface(0, width, height, colorBits, 0, 0, 0, 0);
	}(newWidth, newHeight, Surface::DEFAULT_BPP);

	// Make this surface transparent.
	this->setTransparentColor(Color::Transparent);
	this->fill(Color::Transparent);
	
	// Blit each character surface in at the right spot.
	auto point = Point();
	auto fontSurface = textureManager.getSurface(font.getFontTextureName());
	int upperCharOffsetWidth = font.getUpperCharacterOffsetWidth();
	int upperCharOffsetHeight = font.getUpperCharacterOffsetHeight();
	int lowerCharOffsetWidth = font.getLowerCharacterOffsetWidth();
	int lowerCharOffsetHeight = font.getLowerCharacterOffsetHeight();
	for (auto &line : this->textLines)
	{
		for (auto &c : line)
		{
			int width = islower(c) ? lowerCharWidth : upperCharWidth;
			int height = islower(c) ? lowerCharHeight : upperCharHeight;
			auto letterSurface = [&]()
			{
				auto cellPosition = Font::getCharacterCell(c);
				int offsetWidth = islower(c) ? lowerCharOffsetWidth : upperCharOffsetWidth;
				int offsetHeight = islower(c) ? lowerCharOffsetHeight : upperCharOffsetHeight;

				// Make a copy surface of the font character.
				auto pixelPosition = Point(
					cellPosition.getX() * (width + offsetWidth),
					cellPosition.getY() * (height + offsetHeight));
				auto clipRect = Rectangle(pixelPosition.getX(), pixelPosition.getY(),
					width, height);

				auto surface = Surface(width, height);
				fontSurface.blit(surface, Point(), clipRect);

				return surface;
			}();

			// Set the letter surface to have transparency.
			letterSurface.setTransparentColor(Color::Transparent);

			// Set the letter colors to the desired color.
			auto letterPixels = static_cast<unsigned int*>(letterSurface.getSurface()->pixels);
			auto mappedColor = SDL_MapRGBA(letterSurface.getSurface()->format, color.getR(),
				color.getG(), color.getB(), color.getA());

			int area = letterSurface.getWidth() * letterSurface.getHeight();
			for (int i = 0; i < area; ++i)
			{
				auto pixel = &letterPixels[i];

				// If not transparent, then color it.
				if ((*pixel) != 0)
				{
					*pixel = mappedColor;
				}
			}

			// Draw the letter onto this texture.
			letterSurface.blit(*this, point);

			// Move drawing position to the right of the current character.
			point.setX(point.getX() + width);
		}

		// Move drawing point back to the left and down by one capital character size.
		point.setX(0);
		point.setY(point.getY() + upperCharHeight);
	}

	assert(this->surface->w > 1);
	assert(this->surface->h > 1);
	assert(this->textLines.size() > 0);
	assert(this->fontName == fontName);
}

TextBox::~TextBox()
{

}
