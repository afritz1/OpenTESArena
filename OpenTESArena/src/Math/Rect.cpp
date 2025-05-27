#include "SDL.h"

#include "Rect.h"

bool Rect::isEmpty() const
{
	return (this->width == 0) || (this->height == 0);
}

int Rect::getLeft() const
{
	return this->x;
}

int Rect::getRight() const
{
	return this->getLeft() + this->width;
}

int Rect::getTop() const
{
	return this->y;
}

int Rect::getBottom() const
{
	return this->getTop() + this->height;
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
	return Int2(this->x + (this->width / 2), (this->y + (this->height / 2)));
}

Int2 Rect::getSize() const
{
	return Int2(this->width, this->height);
}

SDL_Rect Rect::getSdlRect() const
{
	SDL_Rect rect;
	rect.x = this->x;
	rect.y = this->y;
	rect.w = this->width;
	rect.h = this->height;
	return rect;
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
