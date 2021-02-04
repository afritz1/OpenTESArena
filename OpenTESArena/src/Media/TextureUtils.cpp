#include "SDL.h"

#include "TextureUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Interface/Surface.h"
#include "../Math/Rect.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

Surface TextureUtils::makeSurfaceFrom8Bit(int width, int height, const uint8_t *pixels, const Palette &palette)
{
	Surface surface = Surface::createWithFormat(width, height, Renderer::DEFAULT_BPP,
		Renderer::DEFAULT_PIXELFORMAT);
	uint32_t *dstPixels = static_cast<uint32_t*>(surface.getPixels());
	const int pixelCount = width * height;

	for (int i = 0; i < pixelCount; i++)
	{
		const uint8_t srcPixel = pixels[i];
		const Color dstColor = palette[srcPixel];
		dstPixels[i] = dstColor.toARGB();
	}

	return surface;
}

Texture TextureUtils::makeTextureFrom8Bit(int width, int height, const uint8_t *pixels,
	const Palette &palette, Renderer &renderer)
{
	Texture texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, width, height);
	if (texture.get() == nullptr)
	{
		DebugLogError("Couldn't create texture (dims: " +
			std::to_string(width) + "x" + std::to_string(height) + ").");
		return texture;
	}

	uint32_t *dstPixels;
	int pitch;
	if (SDL_LockTexture(texture.get(), nullptr, reinterpret_cast<void**>(&dstPixels), &pitch) != 0)
	{
		DebugLogError("Couldn't lock SDL texture (dims: " +
			std::to_string(width) + "x" + std::to_string(height) + ").");
		return texture;
	}

	const int pixelCount = width * height;
	for (int i = 0; i < pixelCount; i++)
	{
		const uint8_t srcPixel = pixels[i];
		const Color dstColor = palette[srcPixel];
		dstPixels[i] = dstColor.toARGB();
	}

	SDL_UnlockTexture(texture.get());

	// Set alpha transparency on.
	if (SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND) != 0)
	{
		DebugLogError("Couldn't set SDL texture alpha blending.");
	}

	return texture;
}

Texture TextureUtils::generate(TextureUtils::PatternType type, int width, int height, TextureManager &textureManager,
	Renderer &renderer)
{
	// Initialize the scratch surface to transparent.
	Surface surface = Surface::createWithFormat(width, height, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	const uint32_t clearColor = surface.mapRGBA(0, 0, 0, 0);
	surface.fill(clearColor);

	if (type == TextureUtils::PatternType::Parchment)
	{
		// Minimum dimensions of parchment pop-up.
		DebugAssert(width >= 40);
		DebugAssert(height >= 40);

		// Get the nine parchment tiles.
		const std::string &tilesPaletteFilename = ArenaTextureName::CharacterCreation;
		const std::optional<PaletteID> tilesPaletteID = textureManager.tryGetPaletteID(tilesPaletteFilename.c_str());
		if (!tilesPaletteID.has_value())
		{
			DebugCrash("Couldn't get tile palette ID for \"" + tilesPaletteFilename + "\".");
		}

		const std::string &tilesFilename = ArenaTextureName::Parchment;
		const std::optional<TextureBuilderIdGroup> tilesTextureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(tilesFilename.c_str());
		if (!tilesTextureBuilderIDs.has_value())
		{
			DebugCrash("Couldn't get tiles texture builder IDs for \"" + tilesFilename + "\".");
		}

		// Lambda for making a temp SDL surface wrapper for writing to the final texture. This is a
		// temp compatibility layer for keeping from changing the SDL blit code below, since making a
		// new surface from a texture builder is wasteful.
		auto makeSurface = [&textureManager, tilesPaletteID](TextureBuilderID textureBuilderID)
		{
			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
			Surface surface = Surface::createWithFormat(textureBuilder.getWidth(), textureBuilder.getHeight(),
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

			// Parchment tiles should all be 8-bit for now.
			DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
			const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
			const Buffer2D<uint8_t> &srcTexels = srcTexture.texels;

			uint32_t *dstPixels = static_cast<uint32_t*>(surface.getPixels());
			const Palette &palette = textureManager.getPaletteHandle(*tilesPaletteID);
			std::transform(srcTexels.get(), srcTexels.end(), dstPixels,
				[&palette](const uint8_t srcTexel)
			{
				return palette[srcTexel].toARGB();
			});

			return surface;
		};

		// Four corner tiles.
		const TextureBuilderID topLeftTextureBuilderID = tilesTextureBuilderIDs->getID(0);
		const TextureBuilderID topRightTextureBuilderID = tilesTextureBuilderIDs->getID(2);
		const TextureBuilderID bottomLeftTextureBuilderID = tilesTextureBuilderIDs->getID(6);
		const TextureBuilderID bottomRightTextureBuilderID = tilesTextureBuilderIDs->getID(8);
		const Surface topLeft = makeSurface(topLeftTextureBuilderID);
		const Surface topRight = makeSurface(topRightTextureBuilderID);
		const Surface bottomLeft = makeSurface(bottomLeftTextureBuilderID);
		const Surface bottomRight = makeSurface(bottomRightTextureBuilderID);

		// Four side tiles.
		const TextureBuilderID topTextureBuilderID = tilesTextureBuilderIDs->getID(1);
		const TextureBuilderID leftTextureBuilderID = tilesTextureBuilderIDs->getID(3);
		const TextureBuilderID rightTextureBuilderID = tilesTextureBuilderIDs->getID(5);
		const TextureBuilderID bottomTextureBuilderID = tilesTextureBuilderIDs->getID(7);
		const Surface top = makeSurface(topTextureBuilderID);
		const Surface left = makeSurface(leftTextureBuilderID);
		const Surface right = makeSurface(rightTextureBuilderID);
		const Surface bottom = makeSurface(bottomTextureBuilderID);

		// One body tile.
		const TextureBuilderID bodyTextureBuilderID = tilesTextureBuilderIDs->getID(4);
		const Surface body = makeSurface(bodyTextureBuilderID);

		// Draw body tiles.
		for (int y = topLeft.getHeight(); y < (surface.getHeight() - topRight.getHeight()); y += body.getHeight())
		{
			for (int x = topLeft.getWidth(); x < (surface.getWidth() - topRight.getWidth()); x += body.getWidth())
			{
				SDL_Rect rect;
				rect.x = x;
				rect.y = y;
				rect.w = body.getWidth();
				rect.h = body.getHeight();

				SDL_BlitSurface(body.get(), nullptr, surface.get(), &rect);
			}
		}

		// Draw edge tiles.
		for (int y = topLeft.getHeight(); y < (surface.getHeight() - bottomLeft.getHeight()); y += left.getHeight())
		{
			const Rect leftRect(0, y, left.getWidth(), left.getHeight());
			const Rect rightRect(surface.getWidth() - right.getWidth(), y, right.getWidth(), right.getHeight());

			// Remove any traces of body tiles underneath.
			surface.fillRect(leftRect, clearColor);
			surface.fillRect(rightRect, clearColor);

			SDL_BlitSurface(left.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&leftRect.getRect()));
			SDL_BlitSurface(right.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&rightRect.getRect()));
		}

		for (int x = topLeft.getWidth(); x < (surface.getWidth() - topRight.getWidth()); x += top.getWidth())
		{
			const Rect topRect(x, 0, top.getWidth(), top.getHeight());
			const Rect bottomRect(x, surface.getHeight() - bottom.getHeight(), bottom.getWidth(), bottom.getHeight());

			// Remove any traces of other tiles underneath.
			surface.fillRect(topRect, clearColor);
			surface.fillRect(bottomRect, clearColor);

			SDL_BlitSurface(top.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&topRect.getRect()));
			SDL_BlitSurface(bottom.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomRect.getRect()));
		}

		// Draw corner tiles.
		const Rect topLeftRect(0, 0, topLeft.getWidth(), topLeft.getHeight());
		const Rect topRightRect(surface.getWidth() - topRight.getWidth(), 0,
			topRight.getWidth(), topRight.getHeight());
		const Rect bottomLeftRect(0, surface.getHeight() - bottomLeft.getHeight(),
			bottomLeft.getWidth(), bottomLeft.getHeight());
		const Rect bottomRightRect(surface.getWidth() - bottomRight.getWidth(),
			surface.getHeight() - bottomRight.getHeight(), bottomRight.getWidth(), bottomRight.getHeight());

		// Remove any traces of other tiles underneath.
		surface.fillRect(topLeftRect, clearColor);
		surface.fillRect(topRightRect, clearColor);
		surface.fillRect(bottomLeftRect, clearColor);
		surface.fillRect(bottomRightRect, clearColor);

		SDL_BlitSurface(topLeft.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&topLeftRect.getRect()));
		SDL_BlitSurface(topRight.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&topRightRect.getRect()));
		SDL_BlitSurface(bottomLeft.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomLeftRect.getRect()));
		SDL_BlitSurface(bottomRight.get(), nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomRightRect.getRect()));
	}
	else if (type == TextureUtils::PatternType::Dark)
	{
		// Minimum dimensions of dark pop-up.
		DebugAssert(width >= 4);
		DebugAssert(height >= 4);

		// Get all the colors used with the dark pop-up.
		const uint32_t fillColor = surface.mapRGBA(28, 24, 36, 255);
		const uint32_t topColor = surface.mapRGBA(36, 36, 48, 255);
		const uint32_t bottomColor = surface.mapRGBA(12, 12, 24, 255);
		const uint32_t rightColor = surface.mapRGBA(56, 69, 77, 255);
		const uint32_t leftColor = bottomColor;
		const uint32_t topRightColor = surface.mapRGBA(69, 85, 89, 255);
		const uint32_t bottomRightColor = surface.mapRGBA(36, 36, 48, 255);

		// Fill with dark-bluish color.
		surface.fill(fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);
		for (int x = 0; x < surface.getWidth(); x++)
		{
			pixels[x] = topColor;
			pixels[x + surface.getWidth()] = topColor;
			pixels[x + ((surface.getHeight() - 2) * surface.getWidth())] = bottomColor;
			pixels[x + ((surface.getHeight() - 1) * surface.getWidth())] = bottomColor;
		}

		for (int y = 0; y < surface.getHeight(); y++)
		{
			pixels[y * surface.getWidth()] = leftColor;
			pixels[1 + (y * surface.getWidth())] = leftColor;
			pixels[(surface.getWidth() - 2) + (y * surface.getWidth())] = rightColor;
			pixels[(surface.getWidth() - 1) + (y * surface.getWidth())] = rightColor;
		}

		// Color corners.
		pixels[1] = topColor;
		pixels[surface.getWidth() - 2] = topColor;
		pixels[surface.getWidth() - 1] = topRightColor;
		pixels[(surface.getWidth() - 2) + surface.getWidth()] = topRightColor;
		pixels[(surface.getWidth() - 2) + ((surface.getHeight() - 2) * surface.getWidth())] = bottomRightColor;
		pixels[(surface.getWidth() - 2) + ((surface.getHeight() - 1) * surface.getWidth())] = bottomColor;
		pixels[(surface.getWidth() - 1) + ((surface.getHeight() - 1) * surface.getWidth())] = bottomRightColor;
	}
	else if (type == TextureUtils::PatternType::Custom1)
	{
		// Minimum dimensions of light-gray pattern.
		DebugAssert(width >= 3);
		DebugAssert(height >= 3);

		const uint32_t fillColor = surface.mapRGBA(85, 85, 97, 255);
		const uint32_t lightBorder = surface.mapRGBA(125, 125, 145, 255);
		const uint32_t darkBorder = surface.mapRGBA(40, 40, 48, 255);

		// Fill with light gray color.
		surface.fill(fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);
		for (int x = 0; x < surface.getWidth(); x++)
		{
			pixels[x] = lightBorder;
			pixels[x + ((surface.getHeight() - 1) * surface.getWidth())] = darkBorder;
		}

		for (int y = 0; y < surface.getHeight(); y++)
		{
			pixels[y * surface.getWidth()] = darkBorder;
			pixels[(surface.getWidth() - 1) + (y * surface.getWidth())] = lightBorder;
		}

		// Color corners.
		pixels[0] = fillColor;
		pixels[(surface.getWidth() - 1) + ((surface.getHeight() - 1) * surface.getWidth())] = fillColor;
	}
	else
	{
		DebugCrash("Unrecognized pattern type.");
	}

	Texture texture = renderer.createTextureFromSurface(surface);
	return texture;
}

std::vector<TextureAssetReference> TextureUtils::makeTextureAssetRefs(const std::string &filename,
	TextureManager &textureManager)
{
	const std::optional<TextureFileMetadata> textureFileMetadata = textureManager.tryGetMetadata(filename.c_str());
	if (!textureFileMetadata.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + filename + "\".");
		return std::vector<TextureAssetReference>();
	}

	std::vector<TextureAssetReference> textureAssetRefs;
	for (int i = 0; i < textureFileMetadata->getTextureCount(); i++)
	{
		TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), i);
		textureAssetRefs.emplace_back(std::move(textureAssetRef));
	}

	return textureAssetRefs;
}
