#include <cassert>

#include "SDL.h"

#include "Surface.h"

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
	SDL_FreeSurface(this->surface);
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

SDL_Surface *Surface::get() const
{
	return this->surface;
}
