#ifndef SURFACE_H
#define SURFACE_H

#include <cstdint>
#include <memory>

// Acts as an SDL_Surface wrapper.

class Color;
class Int2;
class Random;
class Rect;

struct SDL_PixelFormat;
struct SDL_Surface;

class Surface
{
protected:
	SDL_Surface *surface;
	std::unique_ptr<Int2> point;
	bool visible;
public:
	Surface(int x, int y, int width, int height);
	Surface(int width, int height);
	Surface(int x, int y, const SDL_Surface *surface);
	Surface(const SDL_Surface *surface, double scale);
	Surface(const SDL_Surface *surface);
	Surface(const Surface &surface);
	virtual ~Surface();

	// Wrapper function for SDL 2.0.5 "SDL_CreateRGBSurfaceWithFormat()".
	// Remove this function once the project is using 2.0.5 again.
	static SDL_Surface *createSurfaceWithFormat(int width, int height, 
		int depth, uint32_t format);

	// Wrapper function for SDL 2.0.5 "SDL_CreateRGBSurfaceWithFormatFrom()".
	// Remove this function once the project is using 2.0.5 again.
	static SDL_Surface *createSurfaceWithFormatFrom(const uint32_t *pixels,
		int width, int height, int depth, int pitch, uint32_t format);

	static Surface randomNoise(int width, int height, Random &random);

	int getX() const;
	int getY() const;
	int getWidth() const;
	int getHeight() const;
	SDL_Surface *getSurface() const;
	const Int2 &getPoint() const;

	bool isVisible() const;
	bool containsPoint(const Int2 &point);

	void setX(int x);
	void setY(int y);
	void setVisibility(bool visible);
	void optimize(const SDL_PixelFormat *format);
	void setTransparentColor(const Color &color);

	virtual void tick();
	void fill(const Color &color);
	void fillRect(const Rect &rectangle, const Color &color);
	void outline(const Color &color);
	void blit(Surface &dst, const Int2 &dstPoint, const Rect &clipRect) const;
	void blit(Surface &dst, const Int2 &dstPoint) const;
	void blit(Surface &dst) const;
	void blitScaled(Surface &dst, double scale, const Int2 &point,
		const Rect &clipRect) const;
	void blitScaled(Surface &dst, double scale, const Int2 &point) const;
	void blitScaled(Surface &dst, double scale) const;
};

#endif
