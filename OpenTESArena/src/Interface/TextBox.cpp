#include <algorithm>

#include "SDL.h"

#include "TextAlignment.h"
#include "TextBox.h"
#include "../Math/Rect.h"
#include "../Media/FontDefinition.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

TextBox::TextBox(int x, int y, const RichTextString &richText,
	const ShadowData *shadow, const FontLibrary &fontLibrary, Renderer &renderer)
	: richText(richText)
{
	this->x = x;
	this->y = y;

	// Get the font used by the rich text string.
	const char *fontNameStr = FontUtils::fromName(richText.getFontName());
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontIndex))
	{
		DebugCrash("Couldn't get font index \"" + std::string(fontNameStr) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
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

	// Lambda for drawing a font character onto a surface.
	auto blitCharToSurface = [](const FontDefinition::Character &src, int dstX, int dstY, Surface &dst)
	{
		const uint32_t white = dst.mapRGBA(255, 255, 255, 255);
		const uint32_t transparent = dst.mapRGBA(0, 0, 0, 0);

		const FontDefinition::Pixel *srcPixels = src.get();
		uint32_t *dstPixels = static_cast<uint32_t*>(dst.getPixels());

		DebugAssert((dstX + src.getWidth()) <= dst.getWidth());
		DebugAssert((dstY + src.getHeight()) <= dst.getHeight());

		for (int y = 0; y < src.getHeight(); y++)
		{
			for (int x = 0; x < src.getWidth(); x++)
			{
				// Convert font characters (1-bit) to surface format.
				const int srcIndex = x + (y * src.getWidth());
				const int dstIndex = (dstX + x) + ((dstY + y) * dst.getWidth());
				const bool pixelIsSet = srcPixels[srcIndex];
				dstPixels[dstIndex] = pixelIsSet ? white : transparent;
			}
		}
	};

	auto blitSurfaceToSurface = [](const SDL_Surface *src, int dstX, int dstY, Surface &dst)
	{
		SDL_Rect rect;
		rect.x = dstX;
		rect.y = dstY;
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
	auto drawTempText = [&richText, &dimensions, &fontDef, &tempSurface, &blitCharToSurface,
		&setNonTransparentPixels](const Color &color)
	{
		// Get the character surfaces for each line of text.
		const auto &characterLists = richText.getCharacterLists();
		const TextAlignment alignment = richText.getAlignment();

		// Draw each character's surface onto the intermediate surface based on alignment.
		if (alignment == TextAlignment::Left)
		{
			int yOffset = 0;
			for (const std::vector<FontDefinition::CharID> &charIDs : characterLists)
			{
				int xOffset = 0;
				for (const FontDefinition::CharID charID : charIDs)
				{
					const FontDefinition::Character &character = fontDef.getCharacter(charID);
					blitCharToSurface(character, xOffset, yOffset, tempSurface);
					xOffset += character.getWidth();
				}

				yOffset += richText.getCharacterHeight() + richText.getLineSpacing();
			}
		}
		else if (alignment == TextAlignment::Center)
		{
			const std::vector<int> &lineWidths = richText.getLineWidths();
			DebugAssert(lineWidths.size() == characterLists.size());

			int yOffset = 0;
			for (size_t i = 0; i < characterLists.size(); i++)
			{
				const std::vector<FontDefinition::CharID> &charIDs = characterLists[i];
				const int lineWidth = lineWidths[i];
				const int xStart = (dimensions.x / 2) - (lineWidth / 2);

				int xOffset = 0;
				for (const FontDefinition::CharID charID : charIDs)
				{
					const FontDefinition::Character &character = fontDef.getCharacter(charID);
					blitCharToSurface(character, xStart + xOffset, yOffset, tempSurface);
					xOffset += character.getWidth();
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
		blitSurfaceToSurface(tempSurface.get(), std::max(shadowOffset.x, 0),
			std::max(shadowOffset.y, 0), this->surface);

		// Set all shadow pixels in the temp surface to the main color.
		setNonTransparentPixels(richText.getColor());

		blitSurfaceToSurface(tempSurface.get(), std::max(-shadowOffset.x, 0),
			std::max(-shadowOffset.y, 0), this->surface);
	}
	else
	{
		drawTempText(richText.getColor());
		blitSurfaceToSurface(tempSurface.get(), 0, 0, this->surface);
	}

	// Create the destination SDL textures (keeping the surfaces' color keys).
	this->texture = renderer.createTextureFromSurface(this->surface);
}

TextBox::TextBox(const Int2 &center, const RichTextString &richText,
	const ShadowData *shadow, const FontLibrary &fontLibrary, Renderer &renderer)
	: TextBox(center.x, center.y, richText, shadow, fontLibrary, renderer)
{
	// Shift the resulting text box coordinates left and up to center it over
	// the text (ignoring any shadow).
	this->x -= richText.getDimensions().x / 2;
	this->y -= richText.getDimensions().y / 2;
}

TextBox::TextBox(int x, int y, const RichTextString &richText, const FontLibrary &fontLibrary,
	Renderer &renderer)
	: TextBox(x, y, richText, nullptr, fontLibrary, renderer) { }

TextBox::TextBox(const Int2 &center, const RichTextString &richText, 
	const FontLibrary &fontLibrary, Renderer &renderer)
	: TextBox(center, richText, nullptr, fontLibrary, renderer) { }

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
