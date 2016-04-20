#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "Surface.h"

#include "../Media/Color.h"
#include "../Math/Int2.h"
#include "../Math/Random.h"
#include "../Math/Rectangle.h"

const int Surface::DEFAULT_BPP = 32;

Surface::Surface(int x, int y, int width, int height)
{
	assert(width > 0);
	assert(height > 0);

	this->surface = SDL_CreateRGBSurface(0, width, height, Surface::DEFAULT_BPP, 0, 0, 0, 0);
	this->point = std::unique_ptr<Int2>(new Int2(x, y));
	this->visible = true;

	assert(this->surface != nullptr);
	assert(this->point.get() != nullptr);
	assert(this->visible);
}

Surface::Surface(int width, int height)
	: Surface(0, 0, width, height) { }

Surface::Surface(int x, int y, const SDL_Surface *surface)
{
	this->surface = SDL_CreateRGBSurfaceFrom(surface->pixels, surface->w,
		surface->h, Surface::DEFAULT_BPP, surface->pitch, surface->format->Rmask,
		surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	this->point = std::unique_ptr<Int2>(new Int2(x, y));
	this->visible = true;

	assert(this->surface != nullptr);
	assert(this->point.get() != nullptr);
	assert(this->visible);
}

Surface::Surface(const SDL_Surface *surface, double scale)
{
	int width = static_cast<int>(static_cast<double>(surface->w) * scale);
	int height = static_cast<int>(static_cast<double>(surface->h) * scale);
	this->surface = SDL_CreateRGBSurface(0, width, height, Surface::DEFAULT_BPP,
		surface->format->Rmask, surface->format->Gmask, surface->format->Bmask,
		surface->format->Amask);
	auto rect = SDL_Rect();
	rect.w = width;
	rect.h = height;
	SDL_BlitScaled(const_cast<SDL_Surface*>(surface), nullptr, this->surface, &rect);

	this->point = std::unique_ptr<Int2>(new Int2());
	this->visible = true;

	assert(this->surface != nullptr);
	assert(this->point.get() != nullptr);
	assert(this->visible);
}

Surface::Surface(const SDL_Surface *surface)
	: Surface(0, 0, surface) { }

Surface::Surface(const Surface &surface)
	: Surface(surface.getSurface()) { }

Surface::~Surface()
{
	assert(this->surface != nullptr);

	SDL_FreeSurface(this->surface);
}

Surface Surface::randomNoise(int width, int height, Random &random)
{
	auto surface = Surface(width, height);
	auto pixels = static_cast<unsigned int*>(surface.getSurface()->pixels);
	int area = surface.getWidth() * surface.getHeight();
	for (int i = 0; i < area; ++i)
	{
		pixels[i] = Color::randomRGB(random).toRGB();
	}

	assert(surface.getWidth() == width);
	assert(surface.getHeight() == height);

	return surface;
}

int Surface::getX() const
{
	return this->point->getX();
}

int Surface::getY() const
{
	return this->point->getY();
}

int Surface::getWidth() const
{
	return this->surface->w;
}

int Surface::getHeight() const
{
	return this->surface->h;
}

SDL_Surface *Surface::getSurface() const
{
	assert(this->surface != nullptr);

	return this->surface;
}

const Int2 &Surface::getPoint() const
{
	return *this->point;
}

bool Surface::isVisible() const
{
	return this->visible;
}

bool Surface::containsPoint(const Int2 &point)
{
	auto rect = Rectangle(this->point->getX(), this->point->getY(),
		this->getWidth(), this->getHeight());
	return rect.contains(point);
}

void Surface::setX(int x)
{
	this->point->setX(x);
}

void Surface::setY(int y)
{
	this->point->setY(y);
}

void Surface::setVisibility(bool visible)
{
	this->visible = visible;
}

void Surface::optimize(const SDL_PixelFormat *format)
{
	assert(format != nullptr);

	auto optSurface = SDL_ConvertSurface(this->surface, format, this->surface->flags);
	if (optSurface == nullptr)
	{
		std::cerr << "Surface error: could not optimize surface." << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}

	this->surface = optSurface;

	assert(this->surface == optSurface);
}

void Surface::setTransparentColor(const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_SetColorKey(this->surface, SDL_TRUE, mappedColor);
}

void Surface::tick()
{
	// Do nothing. Usually for animating something in a surface, like waves.
	// Not for switching between sprite images.
}

void Surface::fill(const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_FillRect(this->surface, nullptr, mappedColor);
}

void Surface::fillRect(const Rectangle &rectangle, const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_FillRect(this->surface, rectangle.getRect(), mappedColor);
}

void Surface::outline(const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	auto surfacePixels = static_cast<unsigned int*>(this->surface->pixels);

	int width = this->surface->w;
	int height = this->surface->h;

	// Top and bottom rows.
	for (int x = 0; x < width; ++x)
	{
		surfacePixels[x] = mappedColor;
		surfacePixels[x + ((height - 1) * width)] = mappedColor;
	}

	// Left and right columns, ignoring already-written top and bottom pixels.
	for (int y = 1; y < (height - 1); ++y)
	{
		surfacePixels[y * width] = mappedColor;
		surfacePixels[(width - 1) + (y * width)] = mappedColor;
	}
}

void Surface::blit(Surface &dst, const Int2 &point, const Rectangle &clipRect) const
{
	auto rect = SDL_Rect();
	rect.x = point.getX();
	rect.y = point.getY();
	SDL_BlitSurface(this->surface, clipRect.getRect(), dst.getSurface(), &rect);
}

void Surface::blit(Surface &dst, const Int2 &point) const
{
	this->blit(dst, point, Rectangle());
}

void Surface::blit(Surface &dst) const
{
	this->blit(dst, Int2(), Rectangle());
}

void Surface::blitScaled(Surface &dst, double scale, const Int2 &point,
	const Rectangle &clipRect) const
{
	auto scaleRect = SDL_Rect();
	scaleRect.x = point.getX();
	scaleRect.y = point.getY();
	scaleRect.w = static_cast<int>(static_cast<double>(this->surface->w) * scale);
	scaleRect.h = static_cast<int>(static_cast<double>(this->surface->h) * scale);
	SDL_BlitScaled(this->surface, clipRect.getRect(), dst.getSurface(), &scaleRect);
}

void Surface::blitScaled(Surface &dst, double scale, const Int2 &point) const
{
	this->blitScaled(dst, scale, point, Rectangle());
}

void Surface::blitScaled(Surface &dst, double scale) const
{
	this->blitScaled(dst, scale, Int2(), Rectangle());
}
