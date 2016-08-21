#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <memory>

class Int2;

struct SDL_Rect;

class Rect
{
private:
	std::unique_ptr<SDL_Rect> rect;

	bool isEmpty() const;
public:
	Rect(int32_t x, int32_t y, int32_t width, int32_t height);
	Rect(int32_t width, int32_t height);
	Rect();
	Rect(const Rect &rectangle);
	~Rect();

	int32_t getWidth() const;
	int32_t getHeight() const;
	int32_t getLeft() const;
	int32_t getRight() const;
	int32_t getTop() const;
	int32_t getBottom() const;
	Int2 getTopLeft() const;
	Int2 getTopRight() const;
	Int2 getBottomLeft() const;
	Int2 getBottomRight() const;
	Int2 getCenter() const;
	const SDL_Rect *getRect() const;

	void setX(int32_t x);
	void setY(int32_t y);
	void setWidth(int32_t width);
	void setHeight(int32_t height);

	bool contains(const Int2 &point) const;
	bool contains(const Rect &rectangle) const;
	bool intersects(const Rect &rectangle) const;
};

#endif
