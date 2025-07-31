#include "SDL.h"

#include "Surface.h"
#include "../Math/Rect.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

Surface::Surface()
{
	this->surface = nullptr;
}

Surface::Surface(Surface &&surface)
{
	this->surface = surface.surface;
	surface.surface = nullptr;
}

Surface::~Surface()
{
	if (this->surface != nullptr)
	{
		SDL_FreeSurface(this->surface);
	}
}

void Surface::init(SDL_Surface *surface)
{
	DebugAssert(this->surface == nullptr);
	this->surface = surface;
}

Surface &Surface::operator=(Surface &&surface)
{
	if (this->surface != surface.surface)
	{
		if (this->surface != nullptr)
		{
			SDL_FreeSurface(this->surface);
		}

		this->surface = surface.surface;
	}
	
	surface.surface = nullptr;
	return *this;
}

Surface Surface::loadBMP(const char *filename, uint32_t format)
{
	if (String::isNullOrEmpty(filename))
	{
		return Surface();
	}

	SDL_Surface *surface = SDL_LoadBMP(filename);
	if (surface == nullptr)
	{
		DebugLogWarning("Could not load .BMP \"" + std::string(filename) + "\".");
		return Surface();
	}

	// Convert to the given pixel format.
	SDL_Surface *optimizedSurface = SDL_ConvertSurfaceFormat(surface, format, 0);
	SDL_FreeSurface(surface);

	Surface newSurface;
	newSurface.init(optimizedSurface);
	return newSurface;
}

Surface Surface::createWithFormat(int width, int height, int depth, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	Surface surface;
	surface.init(SDL_CreateRGBSurfaceWithFormat(0, width, height, depth, format));
	return surface;
#else
	SDL_Surface *unoptSurface = SDL_CreateRGBSurface(0, width, height, depth, 0, 0, 0, 0);
	SDL_Surface *optSurface = SDL_ConvertSurfaceFormat(unoptSurface, format, 0);
	SDL_FreeSurface(unoptSurface);

	// Surfaces with alpha are set up for blending by default.
	if (optSurface->format->Amask != 0)
	{
		SDL_SetSurfaceBlendMode(optSurface, SDL_BLENDMODE_BLEND);
	}

	Surface newSurface;
	newSurface.init(optSurface);
	return newSurface;
#endif
}

Surface Surface::createWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	Surface surface;
	surface.init(SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, depth, pitch, format));
	return surface;
#else
	Surface surface = Surface::createWithFormat(width, height, depth, format);
	SDL_memcpy(surface.get()->pixels, pixels, height * pitch);
	return surface;
#endif
}

int Surface::getWidth() const
{
	return this->surface->w;
}

int Surface::getHeight() const
{
	return this->surface->h;
}

Span2D<uint32_t> Surface::getPixels()
{
	return Span2D<uint32_t>(reinterpret_cast<uint32_t*>(this->surface->pixels), this->surface->w, this->surface->h);
}

Span2D<const uint32_t> Surface::getPixels() const
{
	return Span2D<const uint32_t>(reinterpret_cast<const uint32_t*>(this->surface->pixels), this->surface->w, this->surface->h);
}

SDL_Surface *Surface::get() const
{
	return this->surface;
}

uint32_t Surface::mapRGB(uint8_t r, uint8_t g, uint8_t b) const
{
	return SDL_MapRGB(this->surface->format, r, g, b);
}

uint32_t Surface::mapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const
{
	return SDL_MapRGBA(this->surface->format, r, g, b, a);
}

void Surface::fill(uint32_t color)
{
	SDL_FillRect(this->surface, nullptr, color);
}

void Surface::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->fill(this->mapRGBA(r, g, b, a));
}

void Surface::fill(uint8_t r, uint8_t g, uint8_t b)
{
	this->fill(r, g, b, 255);
}

void Surface::fillRect(const Rect &rect, uint32_t color)
{
	const SDL_Rect rectSdl = rect.getSdlRect();
	SDL_FillRect(this->surface, &rectSdl, color);
}

void Surface::fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->fillRect(rect, this->mapRGBA(r, g, b, a));
}

void Surface::fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b)
{
	this->fillRect(rect, r, g, b, 255);
}

void Surface::blit(Surface &dst, const Rect &dstRect) const
{
#if SDL_COMPILEDVERSION == SDL_VERSIONNUM(2, 0, 9)
	// Workaround for an issue found with text and parchment rendering on Raspberry Pi 4. I doubt it's an actual
	// SDL bug but this workaround fixes it for some reason (may be ARM64-related?).
	const SDL_Surface *src = this->surface;
	const uint32_t srcTransparent = SDL_MapRGBA(src->format, 0, 0, 0, 0);
	const uint32_t *srcPixels = static_cast<const uint32_t*>(src->pixels);
	uint32_t *dstPixels = static_cast<uint32_t*>(dst.getPixels());
	const int dstX = dstRect.getLeft();
	const int dstY = dstRect.getTop();

	DebugAssert((dstX + src->w) <= dst.getWidth());
	DebugAssert((dstY + src->h) <= dst.getHeight());

	for (int y = 0; y < src->h; y++)
	{
		for (int x = 0; x < src->w; x++)
		{
			const int srcIndex = x + (y * src->w);
			const uint32_t srcPixel = srcPixels[srcIndex];
			if (srcPixel != srcTransparent)
			{
				const int dstIndex = (dstX + x) + ((dstY + y) * dst.getWidth());
				dstPixels[dstIndex] = srcPixel;
			}
		}
	}
#else
	SDL_Rect dstRectSdl = dstRect.getSdlRect();
	SDL_BlitSurface(this->surface, nullptr, dst.surface, &dstRectSdl);
#endif
}

void Surface::blit(Surface &dst, int dstX, int dstY) const
{
	const Rect dstRect(dstX, dstY, this->getWidth(), this->getHeight());
	this->blit(dst, dstRect);
}

void Surface::blitRect(const Rect &srcRect, Surface &dst, const Rect &dstRect) const
{
	const SDL_Rect srcRectSdl = srcRect.getSdlRect();
	SDL_Rect dstRectSdl = dstRect.getSdlRect();
	SDL_BlitSurface(this->surface, &srcRectSdl, dst.surface, &dstRectSdl);
}

void Surface::blitRect(const Rect &srcRect, Surface &dst, int dstX, int dstY) const
{
	const Rect dstRect(dstX, dstY, this->getWidth(), this->getHeight());
	this->blitRect(srcRect, dst, dstRect);
}

void Surface::clear()
{
	if (this->surface != nullptr)
	{
		SDL_FreeSurface(this->surface);
		this->surface = nullptr;
	}
}
