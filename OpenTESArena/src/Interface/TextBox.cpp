#include "SDL.h"

#include "TextAlignment.h"
#include "TextBox.h"
#include "../Math/Rect.h"
#include "../Media/Font.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

TextBox::TextBox(int x, int y, const RichTextString &richText,
	const Color &shadowColor, Renderer &renderer)
	: richText(richText)
{
	this->x = x;
	this->y = y;

	// Get the width and height of the rich text.
	const Int2 &dimensions = richText.getDimensions();

	// Create an intermediate SDL surface for blitting each character surface onto
	// before changing all non-black pixels to the desired text color.
	this->surface = [&dimensions, &renderer]()
	{
		SDL_Surface *surface = Surface::createSurfaceWithFormat(dimensions.x,
			dimensions.y, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

		return surface;
	}();

	// Lambda for drawing a surface onto another surface.
	auto blitToSurface = [](const SDL_Surface *src, int x, int y, SDL_Surface *dst)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = src->w;
		rect.h = src->h;

		SDL_BlitSurface(const_cast<SDL_Surface*>(src), nullptr, dst, &rect);
	};

	// The surface lists are a set of character surfaces for each line of text.
	const auto &surfaceLists = richText.getSurfaceLists();
	const TextAlignment alignment = richText.getAlignment();

	// Draw each character's surface onto the intermediate surface based on alignment.
	if (alignment == TextAlignment::Left)
	{
		int yOffset = 0;
		for (const auto &surfaceList : surfaceLists)
		{
			int xOffset = 0;
			for (auto *surface : surfaceList)
			{
				blitToSurface(surface, xOffset, yOffset, this->surface);
				xOffset += surface->w;
			}

			yOffset += richText.getCharacterHeight() + richText.getLineSpacing();
		}
	}
	else if (alignment == TextAlignment::Center)
	{
		const std::vector<int> &lineWidths = richText.getLineWidths();

		int yOffset = 0;
		for (size_t y = 0; y < surfaceLists.size(); ++y)
		{
			const auto &charSurfaces = surfaceLists.at(y);
			const int lineWidth = lineWidths.at(y);
			const int xStart = (dimensions.x / 2) - (lineWidth / 2);

			int xOffset = 0;
			for (auto *surface : charSurfaces)
			{
				blitToSurface(surface, xStart + xOffset, yOffset, this->surface);
				xOffset += surface->w;
			}

			yOffset += richText.getCharacterHeight() + richText.getLineSpacing();
		}
	}
	else
	{
		DebugCrash("Alignment \"" +
			std::to_string(static_cast<int>(alignment)) + "\" unrecognized.");
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
	const int pixelCount = dimensions.x * dimensions.y;
	const Color &textColor = richText.getColor();
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

TextBox::TextBox(const Int2 &center, const RichTextString &richText,
	const Color &shadowColor, Renderer &renderer)
	: TextBox(center.x, center.y, richText, shadowColor, renderer)
{
	// Just shift the resulting text box coordinates left and up to center it.
	int width, height;
	SDL_QueryTexture(this->texture, nullptr, nullptr, &width, &height);

	this->x -= width / 2;
	this->y -= height / 2;
}

TextBox::TextBox(int x, int y, const RichTextString &richText, Renderer &renderer)
	: TextBox(x, y, richText, Color::Black, renderer) { }

TextBox::TextBox(const Int2 &center, const RichTextString &richText, Renderer &renderer)
	: TextBox(center, richText, Color::Black, renderer) { }

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

const RichTextString &TextBox::getRichText() const
{
	return this->richText;
}

Rect TextBox::getRect() const
{
	return Rect(this->x, this->y, this->surface->w, this->surface->h);
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
