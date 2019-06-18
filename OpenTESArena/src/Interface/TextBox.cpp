#include <algorithm>

#include "SDL.h"

#include "TextAlignment.h"
#include "TextBox.h"
#include "../Math/Rect.h"
#include "../Media/Font.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

TextBox::TextBox(int x, int y, const RichTextString &richText,
	const ShadowData *shadow, Renderer &renderer)
	: richText(richText)
{
	this->x = x;
	this->y = y;

	// Get the width and height of the rich text.
	const Int2 &dimensions = richText.getDimensions();

	// Get the shadow data (if any).
	const bool hasShadow = shadow != nullptr;
	const Color shadowColor = hasShadow ? shadow->color : Color();
	const Int2 shadowOffset = hasShadow ? shadow->offset : Int2();

	// Create an intermediate surface for blitting each character surface onto
	// before changing all non-transparent pixels to the desired text color.
	Surface tempSurface = [&dimensions, &renderer]()
	{
		Surface surface = Surface::createWithFormat(dimensions.x, dimensions.y,
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		surface.fill(0, 0, 0, 0);

		return surface;
	}();

	// Lambda for drawing a surface onto another surface.
	auto blitToSurface = [](const SDL_Surface *src, int x, int y, Surface &dst)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = src->w;
		rect.h = src->h;

		SDL_BlitSurface(const_cast<SDL_Surface*>(src), nullptr, dst.get(), &rect);
	};

	// Lambda for setting all non-transparent pixels in the temp surface to some color.
	auto setNonTransparentPixels = [&tempSurface](const Color &color)
	{
		uint32_t *pixels = static_cast<uint32_t*>(tempSurface.getPixels());
		const int pixelCount = tempSurface.getWidth() * tempSurface.getHeight();
		const uint32_t transparent = tempSurface.mapRGBA(0, 0, 0, 0);
		const uint32_t desiredColor = tempSurface.mapRGBA(color.r, color.g, color.b, color.a);

		std::for_each(pixels, pixels + pixelCount,
			[transparent, desiredColor](uint32_t &pixel)
		{
			if (pixel != transparent)
			{
				pixel = desiredColor;
			}
		});
	};

	// Lambda for drawing the text to the temp surface with some color.
	auto drawTempText = [&richText, &dimensions, &tempSurface, &blitToSurface,
		&setNonTransparentPixels](const Color &color)
	{
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
					blitToSurface(surface, xOffset, yOffset, tempSurface);
					xOffset += surface->w;
				}

				yOffset += richText.getCharacterHeight() + richText.getLineSpacing();
			}
		}
		else if (alignment == TextAlignment::Center)
		{
			const std::vector<int> &lineWidths = richText.getLineWidths();
			DebugAssert(lineWidths.size() == surfaceLists.size());

			int yOffset = 0;
			for (size_t i = 0; i < surfaceLists.size(); i++)
			{
				const auto &charSurfaces = surfaceLists[i];
				const int lineWidth = lineWidths[i];
				const int xStart = (dimensions.x / 2) - (lineWidth / 2);

				int xOffset = 0;
				for (auto *surface : charSurfaces)
				{
					blitToSurface(surface, xStart + xOffset, yOffset, tempSurface);
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

		// Change all non-transparent pixels in the scratch surface to the desired 
		// text colors.
		setNonTransparentPixels(color);
	};

	// Create the text box surface itself with proper dimensions (i.e., accounting for
	// any shadow offset).
	this->surface = [&tempSurface, &shadowOffset]()
	{
		const Int2 surfaceDims(
			tempSurface.getWidth() + std::abs(shadowOffset.x),
			tempSurface.getHeight() + std::abs(shadowOffset.y));

		Surface surface = Surface::createWithFormat(surfaceDims.x, surfaceDims.y,
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		surface.fill(0, 0, 0, 0);

		return surface;
	}();

	// Determine how to fill the text box surface, based on whether it has a shadow.
	if (hasShadow)
	{
		drawTempText(shadowColor);
		blitToSurface(tempSurface.get(), std::max(shadowOffset.x, 0),
			std::max(shadowOffset.y, 0), this->surface);

		// Set all shadow pixels in the temp surface to the main color.
		setNonTransparentPixels(richText.getColor());

		blitToSurface(tempSurface.get(), std::max(-shadowOffset.x, 0),
			std::max(-shadowOffset.y, 0), this->surface);
	}
	else
	{
		drawTempText(richText.getColor());
		blitToSurface(tempSurface.get(), 0, 0, this->surface);
	}

	// Create the destination SDL textures (keeping the surfaces' color keys).
	this->texture = renderer.createTextureFromSurface(this->surface);
}

TextBox::TextBox(const Int2 &center, const RichTextString &richText,
	const ShadowData *shadow, Renderer &renderer)
	: TextBox(center.x, center.y, richText, shadow, renderer)
{
	// Shift the resulting text box coordinates left and up to center it over
	// the text (ignoring any shadow).
	this->x -= richText.getDimensions().x / 2;
	this->y -= richText.getDimensions().y / 2;
}

TextBox::TextBox(int x, int y, const RichTextString &richText, Renderer &renderer)
	: TextBox(x, y, richText, nullptr, renderer) { }

TextBox::TextBox(const Int2 &center, const RichTextString &richText, Renderer &renderer)
	: TextBox(center, richText, nullptr, renderer) { }

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
	return Rect(this->x, this->y, this->surface.get()->w, this->surface.get()->h);
}

const Surface &TextBox::getSurface() const
{
	return this->surface;
}

const Texture &TextBox::getTexture() const
{
	return this->texture;
}
