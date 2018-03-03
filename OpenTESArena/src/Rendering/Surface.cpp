#include <cassert>

#include "SDL.h"

#include "Surface.h"
#include "../Utilities/Debug.h"

Surface::Surface()
{
	this->surface = nullptr;
}

Surface::Surface(SDL_Surface *surface)
{
	assert(surface != nullptr);
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

SDL_Surface *Surface::loadBMP(const std::string &filename, uint32_t format)
{
	SDL_Surface *surface = SDL_LoadBMP(filename.c_str());
	DebugAssert(surface != nullptr, "Could not find \"" + filename + "\".");

	// Convert to the given pixel format.
	SDL_Surface *optimizedSurface = SDL_ConvertSurfaceFormat(surface, format, 0);
	SDL_FreeSurface(surface);

	return optimizedSurface;
}

SDL_Surface *Surface::createSurfaceWithFormat(int width, int height,
	int depth, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, depth, format);
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

	return optSurface;
#endif
}

SDL_Surface *Surface::createSurfaceWithFormatFrom(void *pixels,
	int width, int height, int depth, int pitch, uint32_t format)
{
#if SDL_VERSION_ATLEAST(2, 0, 5)
	return SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height,
		depth, pitch, format);
#else
	SDL_Surface *surface = Surface::createSurfaceWithFormat(width, height, depth, format);
	SDL_memcpy(surface->pixels, pixels, height * pitch);
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

SDL_Surface *Surface::get() const
{
	return this->surface;
}
