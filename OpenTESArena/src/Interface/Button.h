#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"

// A button encapsulates some callback functionality. It usually modifies the
// game state, but could also modify something small in a panel instead.

template <class... Args>
class Button
{
private:
	std::function<void(Args...)> function;
	int x, y, width, height;
public:
	Button(int x, int y, int width, int height,
		const std::function<void(Args...)> &function)
	{
		this->function = function;
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

	Button(const Int2 &center, int width, int height,
		const std::function<void(Args...)> &function)
		: Button(center.x - (width / 2), center.y - (height / 2),
			width, height, function) { }

	// "Hidden" button, intended only as a hotkey.
	Button(const std::function<void(Args...)> &function)
		: Button(0, 0, 1, 1, function) { }

	virtual ~Button()
	{

	}

	int getX() const
	{
		return this->x;
	}

	int getY() const
	{
		return this->y;
	}

	int getWidth() const
	{
		return this->width;
	}

	int getHeight() const
	{
		return this->height;
	}

	// Returns whether the button's area contains the given point.
	bool contains(const Int2 &point)
	{
		Rect rect(this->x, this->y, this->width, this->height);
		return rect.contains(point);
	}

	// Calls the button's function.
	void click(Args... args)
	{
		this->function(args...);
	}
};

#endif
