#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>

// Acts as an SDL_Surface wrapper.

// The usage of SDL_Surfaces in the program is falling in favor of SDL_Textures.
// SDL_Surfaces are really just being used as scratch images which are then converted 
// to a hardware texture for rendering.

struct SDL_Surface;

class Surface
{
private:
	SDL_Surface *surface;
public:
	Surface(SDL_Surface *surface);
	Surface(const Surface&) = delete;
	Surface(Surface &&surface);
	~Surface();

	Surface &operator=(const Surface&) = delete;
	Surface &operator=(Surface&&) = delete;

	// Wrapper function for SDL 2.0.5 "SDL_CreateRGBSurfaceWithFormat()".
	// Remove this function once the project is using 2.0.5 again.
	static SDL_Surface *createSurfaceWithFormat(int width, int height,
		int depth, uint32_t format);

	// Wrapper function for SDL 2.0.5 "SDL_CreateRGBSurfaceWithFormatFrom()".
	// Remove this function once the project is using 2.0.5 again.
	static SDL_Surface *createSurfaceWithFormatFrom(void *pixels,
		int width, int height, int depth, int pitch, uint32_t format);

	SDL_Surface *get() const;
};

#endif
