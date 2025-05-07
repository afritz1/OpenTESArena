#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "../Math/Vector2.h"

struct SDL_Rect;

struct Rect
{
	int x, y, width, height;

	Rect(int x, int y, int width, int height);
	Rect(const Int2 &center, int width, int height);
	Rect(int width, int height);
	Rect();

	bool isEmpty() const;
	int getLeft() const;
	int getRight() const;
	int getTop() const;
	int getBottom() const;
	Int2 getTopLeft() const;
	Int2 getTopRight() const;
	Int2 getBottomLeft() const;
	Int2 getBottomRight() const;
	Int2 getCenter() const;
	Int2 getSize() const;
	SDL_Rect getSdlRect() const;

	bool contains(const Int2 &point) const;
	bool contains(const Rect &rectangle) const;
	bool containsInclusive(const Int2 &point) const;
	bool containsInclusive(const Rect &rectangle) const;
	bool intersects(const Rect &rectangle) const;
};

#endif
