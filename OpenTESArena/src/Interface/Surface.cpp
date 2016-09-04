#include <cassert>
#include <cstring>

#include "SDL.h"

#include "Surface.h"

#include "../Math/Int2.h"
#include "../Math/Random.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"

Surface::Surface(int x, int y, int width, int height)
{
	assert(width > 0);
	assert(height > 0);

	this->surface = SDL_CreateRGBSurface(0, width, height, Renderer::DEFAULT_BPP, 0, 0, 0, 0);
	Debug::check(this->surface != nullptr, "Surface",
		"Insufficient memory in Surface(int, int, int, int).");

	this->point = std::unique_ptr<Int2>(new Int2(x, y));
	this->visible = true;
}

Surface::Surface(int width, int height)
	: Surface(0, 0, width, height) { }

Surface::Surface(int x, int y, const SDL_Surface *surface)
{
	this->surface = SDL_CreateRGBSurface(surface->flags, surface->w, surface->h,
		Renderer::DEFAULT_BPP, surface->format->Rmask, surface->format->Gmask,
		surface->format->Bmask, surface->format->Amask);
	Debug::check(this->surface != nullptr, "Surface",
		"Insufficient memory in Surface(int, int, const SDL_Surface*).");

	std::memcpy(this->surface->pixels, surface->pixels,
		surface->w * surface->h * (Renderer::DEFAULT_BPP / 8));

	this->point = std::unique_ptr<Int2>(new Int2(x, y));
	this->visible = true;
}

Surface::Surface(const SDL_Surface *surface, double scale)
{
	int width = static_cast<int>(static_cast<double>(surface->w) * scale);
	int height = static_cast<int>(static_cast<double>(surface->h) * scale);
	this->surface = SDL_CreateRGBSurface(0, width, height, Renderer::DEFAULT_BPP,
		surface->format->Rmask, surface->format->Gmask, surface->format->Bmask,
		surface->format->Amask);
	Debug::check(this->surface != nullptr, "Surface",
		"Insufficient memory in Surface(const SDL_Surface*, double).");

	SDL_Rect rect;
	rect.w = width;
	rect.h = height;
	SDL_BlitScaled(const_cast<SDL_Surface*>(surface), nullptr, this->surface, &rect);

	this->point = std::unique_ptr<Int2>(new Int2());
	this->visible = true;
}

Surface::Surface(const SDL_Surface *surface)
	: Surface(0, 0, surface) { }

Surface::Surface(const Surface &surface)
	: Surface(surface.getSurface()) { }

Surface::~Surface()
{
	SDL_FreeSurface(this->surface);
}

Surface Surface::randomNoise(int width, int height, Random &random)
{
	Surface surface(width, height);
	auto *pixels = static_cast<unsigned int*>(surface.getSurface()->pixels);
	int area = surface.getWidth() * surface.getHeight();
	for (int i = 0; i < area; ++i)
	{
		pixels[i] = Color::randomRGB(random).toRGB();
	}

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
	Rect rect(this->point->getX(), this->point->getY(),
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
	auto *optSurface = SDL_ConvertSurface(this->surface, format, this->surface->flags);
	Debug::check(optSurface != nullptr, "Surface", "Could not optimize surface.");

	// Get rid of the old surface (this was once a hard-to-find memory leak!).
	SDL_FreeSurface(this->surface);

	this->surface = optSurface;
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

void Surface::fillRect(const Rect &rectangle, const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_FillRect(this->surface, rectangle.getRect(), mappedColor);
}

void Surface::outline(const Color &color)
{
	auto mappedColor = SDL_MapRGBA(this->surface->format, color.getR(), color.getG(),
		color.getB(), color.getA());
	auto *surfacePixels = static_cast<unsigned int*>(this->surface->pixels);

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

void Surface::blit(Surface &dst, const Int2 &dstPoint, const Rect &clipRect) const
{
	SDL_Rect dstRect;
	dstRect.x = dstPoint.getX();
	dstRect.y = dstPoint.getY();
	SDL_BlitSurface(this->surface, clipRect.getRect(), dst.getSurface(), &dstRect);
}

void Surface::blit(Surface &dst, const Int2 &dstPoint) const
{
	this->blit(dst, dstPoint, Rect());
}

void Surface::blit(Surface &dst) const
{
	this->blit(dst, Int2(), Rect());
}

void Surface::blitScaled(Surface &dst, double scale, const Int2 &point,
	const Rect &clipRect) const
{
	SDL_Rect scaleRect;
	scaleRect.x = point.getX();
	scaleRect.y = point.getY();
	scaleRect.w = static_cast<int>(static_cast<double>(this->surface->w) * scale);
	scaleRect.h = static_cast<int>(static_cast<double>(this->surface->h) * scale);
	SDL_BlitScaled(this->surface, clipRect.getRect(), dst.getSurface(), &scaleRect);
}

void Surface::blitScaled(Surface &dst, double scale, const Int2 &point) const
{
	this->blitScaled(dst, scale, point, Rect());
}

void Surface::blitScaled(Surface &dst, double scale) const
{
	this->blitScaled(dst, scale, Int2(), Rect());
}
