#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <memory>

class Point;
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
	class Point getTopLeft() const;
	class Point getTopRight() const;
	class Point getBottomLeft() const;
	class Point getBottomRight() const;
	class Point getCenter() const;
	const SDL_Rect *getRect() const;

	void setX(int x);
	void setY(int y);
	void setWidth(int width);
	void setHeight(int height);

	bool contains(const Point &point);
	bool contains(const Rectangle &rectangle);
	bool intersects(const Rectangle &rectangle);
};

#endif