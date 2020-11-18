#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>

// A thin SDL_Surface wrapper.

// The usage of SDL_Surfaces in the program is falling in favor of SDL_Textures.
// SDL_Surfaces are really just being used as scratch images which are then converted 
// to a hardware texture for rendering.

class Rect;

struct SDL_Surface;

class Surface
{
private:
	SDL_Surface *surface;
public:
	Surface();
	Surface(const Surface&) = delete;
	Surface(Surface &&surface);
	~Surface();

	// Alternative to constructor to avoid accidentally copying pointers and double-freeing, etc..
	// Most code shouldn't touch a native surface directly.
	void init(SDL_Surface *surface);

	Surface &operator=(const Surface&) = delete;
	Surface &operator=(Surface &&surface);

	// Wrapper function for SDL_LoadBMP(). Also converts to the given pixel format. Returns null
	// on failure.
	static Surface loadBMP(const char *filename, uint32_t format);

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

	void fill(uint32_t color);
	void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void fill(uint8_t r, uint8_t g, uint8_t b);
	void fillRect(const Rect &rect, uint32_t color);
	void fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void fillRect(const Rect &rect, uint8_t r, uint8_t g, uint8_t b);

	void blit(Surface &dst, const Rect &dstRect) const;
	void blit(Surface &dst, int dstX, int dstY) const;
	void blitRect(const Rect &srcRect, Surface &dst, const Rect &dstRect) const;
	void blitRect(const Rect &srcRect, Surface &dst, int dstX, int dstY) const;

	void clear();
};

#endif
