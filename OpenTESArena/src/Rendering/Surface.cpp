#include "SDL.h"

#include "Surface.h"
#include "../Math/Rect.h"
#include "../Utilities/Debug.h"

Surface::Surface()
{
	this->surface = nullptr;
}

Surface::Surface(SDL_Surface *surface)
{
	DebugAssert(surface != nullptr);
	this->surface = surface;
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

Surface &Surface::operator=(Surface &&surface)
{
	this->surface = surface.surface;
	surface.surface = nullptr;
	return *this;
}

Surface Surface::loadBMP(const char *filename, uint32_t format)
{
	DebugAssert(filename != nullptr);

	SDL_Surface *surface = SDL_LoadBMP(filename);
	DebugAssertMsg(surface != nullptr, "Could not find \"" + std::string(filename) + "\".");

	// Convert to the given pixel format.
	SDL_Surface *optimizedSurface = SDL_ConvertSurfaceFormat(surface, format, 0);
	SDL_FreeSurface(surface);

	return Surface(optimizedSurface);
}

Surface Surface::createWithFormat(int width, int height, int depth, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	return Surface(SDL_CreateRGBSurfaceWithFormat(0, width, height, depth, format));
#else
	SDL_Surface *unoptSurface = SDL_CreateRGBSurface(0, width, height,
		depth, 0, 0, 0, 0);
	SDL_Surface *optSurface = SDL_ConvertSurfaceFormat(unoptSurface, format, 0);
	SDL_FreeSurface(unoptSurface);

	// Surfaces with alpha are set up for blending by default.
	if (optSurface->format->Amask != 0)
	{
		SDL_SetSurfaceBlendMode(optSurface, SDL_BLENDMODE_BLEND);
	}

	return Surface(optSurface);
#endif
}

Surface Surface::createWithFormatFrom(void *pixels, int width, int height,
	int depth, int pitch, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	return Surface(SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height,
		depth, pitch, format));
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

void *Surface::getPixels() const
{
	return this->surface->pixels;
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
	SDL_FillRect(this->surface, &rect.getRect(), color);
}

void Surface::fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->fillRect(rect, this->mapRGBA(r, g, b, a));
}

void Surface::fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b)
{
	this->fillRect(rect, r, g, b, 255);
}
