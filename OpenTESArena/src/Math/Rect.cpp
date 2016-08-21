#include <cassert>

#include "SDL.h"

#include "Rect.h"

#include "Int2.h"

Rect::Rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
	assert(width >= 0);
	assert(height >= 0);

	this->rect = std::unique_ptr<SDL_Rect>(new SDL_Rect());
	this->rect->x = x;
	this->rect->y = y;
	this->rect->w = width;
	this->rect->h = height;
}

Rect::Rect(int32_t width, int32_t height)
	: Rect(0, 0, width, height) { }

Rect::Rect()
	: Rect(0, 0, 0, 0) { }

Rect::Rect(const Rect &rectangle)
	: Rect(rectangle.rect->x, rectangle.rect->y, rectangle.rect->w, rectangle.rect->h) { }

Rect::~Rect()
{

}

int32_t Rect::getWidth() const
{
	return this->rect->w;
}

int32_t Rect::getHeight() const
{
	return this->rect->h;
}

int32_t Rect::getLeft() const
{
	return this->rect->x;
}

int32_t Rect::getRight() const
{
	return this->getLeft() + this->getWidth();
}

int32_t Rect::getTop() const
{
	return this->rect->y;
}

int32_t Rect::getBottom() const
{
	return this->getTop() + this->getHeight();
}

Int2 Rect::getTopLeft() const
{
	return Int2(this->getLeft(), this->getTop());
}

Int2 Rect::getTopRight() const
{
	return Int2(this->getRight(), this->getTop());
}

Int2 Rect::getBottomLeft() const
{
	return Int2(this->getLeft(), this->getBottom());
}

Int2 Rect::getBottomRight() const
{
	return Int2(this->getRight(), this->getBottom());
}

Int2 Rect::getCenter() const
{
	return Int2(this->getLeft() + (this->getWidth() / 2),
		(this->getTop() + (this->getHeight() / 2)));
}

bool Rect::isEmpty() const
{
	return (this->getLeft() == 0) && (this->getRight() == 0) &&
		(this->getWidth() == 0) && (this->getHeight() == 0);
}

const SDL_Rect *Rect::getRect() const
{
	return this->isEmpty() ? nullptr : this->rect.get();
}

void Rect::setX(int32_t x)
{
	this->rect->x = x;
}

void Rect::setY(int32_t y)
{
	this->rect->y = y;
}

void Rect::setWidth(int32_t width)
{
	this->rect->w = width;
}

void Rect::setHeight(int32_t height)
{
	this->rect->h = height;
}

bool Rect::contains(const Int2 &point) const
{
	return (point.getX() >= this->getLeft()) &&
		(point.getY() >= this->getTop()) &&
		(point.getX() < this->getRight()) &&
		(point.getY() < this->getBottom());
}

bool Rect::contains(const Rect &rectangle) const
{
	return (rectangle.getLeft() >= this->getLeft()) &&
		(rectangle.getTop() >= this->getTop()) &&
		(rectangle.getRight() < this->getRight()) &&
		(rectangle.getBottom() < this->getBottom());
}

bool Rect::intersects(const Rect &rectangle) const
{
	return !((rectangle.getLeft() <= this->getRight()) &&
		(rectangle.getRight() >= this->getLeft()) &&
		(rectangle.getTop() >= this->getBottom()) &&
		(rectangle.getBottom() <= this->getTop()));
}
