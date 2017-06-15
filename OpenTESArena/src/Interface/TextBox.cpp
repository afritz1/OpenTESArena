#include "SDL.h"

#include "TextBox.h"

#include "TextAlignment.h"
#include "../Math/Rect.h"
#include "../Media/Font.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

TextBox::TextBox(int x, int y, const Color &textColor, const Color &shadowColor, 
	const std::string &text, const Font &font, TextAlignment alignment, Renderer &renderer)
	: textColor(textColor)
{
	this->x = x;
	this->y = y;
	this->fontName = font.getFontName();
	this->alignment = alignment;

	// Split "text" into separate lines of text. Also, if "text" is empty, then just 
	// add a space so there doesn't need to be any "zero-character" special cases.
	std::vector<std::string> textLines = String::split(
		(text.size() > 0) ? text : text + " ", '\n');

	// Retrieve a pointer for each character's surface in each line.
	std::vector<std::vector<SDL_Surface*>> lineSurfaces;

	for (const auto &textLine : textLines)
	{
		// Start a new line.
		lineSurfaces.push_back(std::vector<SDL_Surface*>());

		// Get the surface for each character in the current line.
		for (const char c : textLine)
		{
			SDL_Surface *charSurface = font.getSurface(c);
			lineSurfaces.at(lineSurfaces.size() - 1).push_back(charSurface);
		}
	}

	// Get the width of each line in pixels (for determining the longest line).
	std::vector<int> lineWidths;

	for (const auto &charSurfaces : lineSurfaces)
	{
		// Start a new count on the line widths.
		lineWidths.push_back(0);

		// Get the combined widths for the current line's surfaces.
		for (const auto *surface : charSurfaces)
		{
			lineWidths.at(lineWidths.size() - 1) += surface->w;
		}
	}

	// Get the width in pixels for the final SDL texture.
	const int textureWidth = [lineWidths]()
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

	// Get the height in pixels for the final SDL texture.
	const int textureHeight = characterHeight * static_cast<int>(textLines.size());

	// Create an intermediate SDL surface for blitting each character surface onto
	// before changing all non-black pixels to the desired text color.
	this->surface = [textureWidth, textureHeight, &renderer]()
	{
		SDL_Surface *surface = Surface::createSurfaceWithFormat(textureWidth,
			textureHeight, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

		return surface;
	}();

	// Lambda for drawing a surface onto another surface.
	auto blitToSurface = [](SDL_Surface *src, int x, int y, SDL_Surface* dst)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = src->w;
		rect.h = src->h;

		SDL_BlitSurface(src, nullptr, dst, &rect);
	};

	// Draw each character's surface onto the intermediate surface based on alignment.
	if (alignment == TextAlignment::Left)
	{
		int yOffset = 0;
		for (const auto &charSurfaces : lineSurfaces)
		{
			int xOffset = 0;
			for (auto *surface : charSurfaces)
			{
				blitToSurface(surface, xOffset, yOffset, this->surface);
				xOffset += surface->w;
			}

			yOffset += characterHeight;
		}
	}
	else if (alignment == TextAlignment::Center)
	{
		int yOffset = 0;
		for (size_t y = 0; y < lineSurfaces.size(); ++y)
		{
			const auto &charSurfaces = lineSurfaces.at(y);
			const int lineWidth = lineWidths.at(y);
			const int xStart = (textureWidth / 2) - (lineWidth / 2);

			int xOffset = 0;
			for (auto *surface : charSurfaces)
			{
				blitToSurface(surface, xStart + xOffset, yOffset, this->surface);
				xOffset += surface->w;
			}

			yOffset += characterHeight;
		}
	}
	else
	{
		DebugCrash("Alignment \"" + std::to_string(static_cast<int>(alignment)) +
			"\" unrecognized.");
	}

	// Make a temporary surface for use with the shadow texture.
	SDL_Surface *shadowSurface = [this]()
	{
		SDL_Surface *tempSurface = Surface::createSurfaceWithFormat(
			this->surface->w, this->surface->h, 
			this->surface->format->BitsPerPixel, this->surface->format->format);
		SDL_memcpy(tempSurface->pixels, this->surface->pixels,
			this->surface->h * this->surface->pitch);

		return tempSurface;
	}();

	// Change all non-black pixels in the scratch SDL surfaces to the desired 
	// text colors.
	uint32_t *pixels = static_cast<uint32_t*>(this->surface->pixels);
	uint32_t *shadowPixels = static_cast<uint32_t*>(shadowSurface->pixels);
	const int pixelCount = textureWidth * textureHeight;
	const uint32_t black = SDL_MapRGBA(this->surface->format, 0, 0, 0, 0);
	const uint32_t desiredColor = SDL_MapRGBA(this->surface->format, textColor.r,
		textColor.g, textColor.b, textColor.a);
	const uint32_t desiredShadowColor = SDL_MapRGBA(shadowSurface->format, 
		shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);

	for (int i = 0; i < pixelCount; ++i)
	{
		if (pixels[i] != black)
		{
			pixels[i] = desiredColor;
			shadowPixels[i] = desiredShadowColor;
		}
	}

	// Create the destination SDL textures (keeping the surfaces' color keys).
	this->texture = renderer.createTextureFromSurface(this->surface);
	this->shadowTexture = renderer.createTextureFromSurface(shadowSurface);

	SDL_FreeSurface(shadowSurface);
}

TextBox::TextBox(const Int2 &center, const Color &textColor, const Color &shadowColor, 
	const std::string &text, const Font &font, TextAlignment alignment, Renderer &renderer)
	: TextBox(center.x, center.y, textColor, shadowColor, text, font, alignment, renderer)
{
	// Just shift the resulting text box coordinates left and up to center it.
	int width, height;
	SDL_QueryTexture(this->texture, nullptr, nullptr, &width, &height);

	this->x -= width / 2;
	this->y -= height / 2;
}

TextBox::TextBox(int x, int y, const Color &textColor, const std::string &text,
	const Font &font, TextAlignment alignment, Renderer &renderer)
	: TextBox(x, y, textColor, Color::Black, text, font, alignment, renderer) { }

TextBox::TextBox(const Int2 &center, const Color &textColor, const std::string &text,
	const Font &font, TextAlignment alignment, Renderer &renderer)
	: TextBox(center, textColor, Color::Black, text, font, alignment, renderer) { }

TextBox::~TextBox()
{
	SDL_FreeSurface(this->surface);
	SDL_DestroyTexture(this->texture);
	SDL_DestroyTexture(this->shadowTexture);
}

int TextBox::getX() const
{
	return this->x;
}

int TextBox::getY() const
{
	return this->y;
}

TextAlignment TextBox::getAlignment() const
{
	return this->alignment;
}

FontName TextBox::getFontName() const
{
	return this->fontName;
}

const Color &TextBox::getTextColor() const
{
	return this->textColor;
}

SDL_Surface *TextBox::getSurface() const
{
	return this->surface;
}

SDL_Texture *TextBox::getTexture() const
{
	return this->texture;
}

SDL_Texture *TextBox::getShadowTexture() const
{
	return this->shadowTexture;
}
