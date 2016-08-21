#ifndef SURFACE_H
#define SURFACE_H

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
	Surface(int32_t x, int32_t y, int32_t width, int32_t height);
	Surface(int32_t width, int32_t height);
	Surface(int32_t x, int32_t y, const SDL_Surface *surface);
	Surface(const SDL_Surface *surface, double scale);
	Surface(const SDL_Surface *surface);
	Surface(const Surface &surface);
	virtual ~Surface();

	static const int32_t DEFAULT_BPP;

	static Surface randomNoise(int32_t width, int32_t height, Random &random);

	int32_t getX() const;
	int32_t getY() const;
	int32_t getWidth() const;
	int32_t getHeight() const;
	SDL_Surface *getSurface() const;
	const Int2 &getPoint() const;

	bool isVisible() const;
	bool containsPoint(const Int2 &point);

	void setX(int32_t x);
	void setY(int32_t y);
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
