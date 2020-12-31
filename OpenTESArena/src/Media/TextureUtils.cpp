#include "SDL.h"

#include "TextureUtils.h"
#include "../Interface/Surface.h"
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
