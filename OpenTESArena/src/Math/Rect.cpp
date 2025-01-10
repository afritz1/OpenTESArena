#include "SDL.h"

#include "Rect.h"

#include "components/debug/Debug.h"

Rect::Rect(int x, int y, int width, int height)
{
	DebugAssert(width >= 0);
	DebugAssert(height >= 0);

	this->x = x;
	this->y = y;
	this->w = width;
	this->h = height;
}

Rect::Rect(const Int2 &center, int width, int height)
	: Rect(center.x - (width / 2), center.y - (height / 2), width, height) { }

Rect::Rect(int width, int height)
	: Rect(0, 0, width, height) { }

Rect::Rect()
	: Rect(0, 0, 0, 0) { }

Rect::Rect(const Rect &rect)
	: Rect(rect.x, rect.y, rect.w, rect.h) { }

bool Rect::isEmpty() const
{
	return (this->w == 0) || (this->h == 0);
}

int Rect::getWidth() const
{
	return this->w;
}

int Rect::getHeight() const
{
	return this->h;
}

int Rect::getLeft() const
{
	return this->x;
}

int Rect::getRight() const
{
	return this->getLeft() + this->getWidth();
}

int Rect::getTop() const
{
	return this->y;
}

int Rect::getBottom() const
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
	return Int2(this->getLeft() + (this->getWidth() / 2), (this->getTop() + (this->getHeight() / 2)));
}

SDL_Rect Rect::getSdlRect() const
{
	SDL_Rect rect;
	rect.x = this->x;
	rect.y = this->y;
	rect.w = this->w;
	rect.h = this->h;
	return rect;
}

void Rect::setX(int x)
{
	this->x = x;
}

void Rect::setY(int y)
{
	this->y = y;
}

void Rect::setWidth(int width)
{
	this->w = width;
}

void Rect::setHeight(int height)
{
	this->h = height;
}

bool Rect::contains(const Int2 &point) const
{
	return (point.x >= this->getLeft()) &&
		(point.y >= this->getTop()) &&
		(point.x < this->getRight()) &&
		(point.y < this->getBottom());
}

bool Rect::contains(const Rect &rectangle) const
{
	return (rectangle.getLeft() >= this->getLeft()) &&
		(rectangle.getTop() >= this->getTop()) &&
		(rectangle.getRight() < this->getRight()) &&
		(rectangle.getBottom() < this->getBottom());
}

bool Rect::containsInclusive(const Int2 &point) const
{
	return (point.x >= this->getLeft()) &&
		(point.y >= this->getTop()) &&
		(point.x <= this->getRight()) &&
		(point.y <= this->getBottom());
}

bool Rect::containsInclusive(const Rect &rectangle) const
{
	return (rectangle.getLeft() >= this->getLeft()) &&
		(rectangle.getTop() >= this->getTop()) &&
		(rectangle.getRight() <= this->getRight()) &&
		(rectangle.getBottom() <= this->getBottom());
}

bool Rect::intersects(const Rect &rectangle) const
{
	return !((rectangle.getLeft() <= this->getRight()) &&
		(rectangle.getRight() >= this->getLeft()) &&
		(rectangle.getTop() >= this->getBottom()) &&
		(rectangle.getBottom() <= this->getTop()));
}
