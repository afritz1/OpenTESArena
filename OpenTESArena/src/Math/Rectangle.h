#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <memory>

class Int2;

struct SDL_Rect;

class Rectangle
{
private:
	std::unique_ptr<SDL_Rect> rect;

	bool isEmpty() const;
public:
	Rectangle(int x, int y, int width, int height);
	Rectangle(int width, int height);
	Rectangle();
	Rectangle(const Rectangle &rectangle);
	~Rectangle();

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
	const SDL_Rect *getRect() const;

	void setX(int x);
	void setY(int y);
	void setWidth(int width);
	void setHeight(int height);

	bool contains(const Int2 &point) const;
	bool contains(const Rectangle &rectangle) const;
	bool intersects(const Rectangle &rectangle) const;
};

#endif