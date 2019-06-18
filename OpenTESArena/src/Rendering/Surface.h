#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>
#include <string>

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
	Surface();
	Surface(SDL_Surface *surface);
	Surface(const Surface&) = delete;
	Surface(Surface &&surface);
	~Surface();

	Surface &operator=(const Surface&) = delete;
	Surface &operator=(Surface &&surface);

	// Wrapper function for SDL_LoadBMP(). Also converts to the given pixel format.
	static Surface loadBMP(const std::string &filename, uint32_t format);

	// Wrapper function for SDL_CreateRGBSurfaceWithFormat() in SDL 2.0.5.
	static Surface createWithFormat(int width, int height, int depth, uint32_t format);

	// Wrapper function for SDL_CreateRGBSurfaceWithFormatFrom() in SDL 2.0.5.
	static Surface createWithFormatFrom(void *pixels, int width, int height,
		int depth, int pitch, uint32_t format);

	int getWidth() const;
	int getHeight() const;
	void *getPixels() const;
	SDL_Surface *get() const;

	uint32_t mapRGB(uint8_t r, uint8_t g, uint8_t b) const;
	uint32_t mapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const;
};

#endif
