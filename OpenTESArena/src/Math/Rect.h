#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "../Math/Vector2.h"

struct SDL_Rect;

class Rect
{
private:
	int x, y, w, h;
public:
	Rect(int x, int y, int width, int height);
	Rect(const Int2 &center, int width, int height);
	Rect(int width, int height);
	Rect();
	Rect(const Rect &rect);

	bool isEmpty() const;
	int getWidth() const;
	int getHeight() const;
	int getLeft() const;
	int getRight() const;
	int getTop() const;
	int getBottom() const;
	Int2 getTopLeft() const;
	Int2 getTopRight() const;
	Int2 getBottomLeft() const;
	Int2 getBottomRight() const;
	Int2 getCenter() const;
	SDL_Rect getSdlRect() const;

	void setX(int x);
	void setY(int y);
	void setWidth(int width);
	void setHeight(int height);

	bool contains(const Int2 &point) const;
	bool contains(const Rect &rectangle) const;
	bool containsInclusive(const Int2 &point) const;
	bool containsInclusive(const Rect &rectangle) const;
	bool intersects(const Rect &rectangle) const;
};

#endif
