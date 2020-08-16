#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"

// A button encapsulates some callback functionality. It usually modifies the
// game state in some way, but could also modify something in a panel instead.

template <class... Args>
class Button
{
private:
	std::function<void(Args...)> callback;
	Rect rect;
public:
	Button(int x, int y, int width, int height,
		const std::function<void(Args...)> &callback)
		: callback(callback), rect(x, y, width, height) { }

	Button(const Int2 &center, int width, int height,
		const std::function<void(Args...)> &callback)
		: Button(center.x - (width / 2), center.y - (height / 2),
			width, height, callback) { }

	// "Hidden" button, intended only as a hotkey.
	Button(const std::function<void(Args...)> &callback)
		: Button(0, 0, 0, 0, callback) { }

	// Empty button with no dimensions or callback, to be set later.
	Button()
		: Button(0, 0, 0, 0, std::function<void(Args...)>()) { }

	Button(Button&&) = default;

	Button &operator=(Button&&) = default;

	int getX() const
	{
		return this->rect.getLeft();
	}

	int getY() const
	{
		return this->rect.getTop();
	}

	int getWidth() const
	{
		return this->rect.getWidth();
	}

	int getHeight() const
	{
		return this->rect.getHeight();
	}

	// Returns whether the button's area contains the given point.
	bool contains(const Int2 &point) const
	{
		return this->rect.contains(point);
	}

	void setX(int x)
	{
		this->rect.setX(x);
	}

	void setY(int y)
	{
		this->rect.setY(y);
	}

	void setWidth(int width)
	{
		this->rect.setWidth(width);
	}

	void setHeight(int height)
	{
		this->rect.setHeight(height);
	}

	// Sets the button's callback to the given function.
	void setCallback(const std::function<void(Args...)> &fn)
	{
		this->callback = fn;
	}

	// Calls the button's function.
	void click(Args... args)
	{
		this->callback(std::forward<Args>(args)...);
	}
};

#endif
