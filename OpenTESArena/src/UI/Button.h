#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include "../Input/PointerTypes.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"

// A button encapsulates some callback functionality. It usually modifies the
// game state in some way, but could also modify something in a panel instead.
template<class... Args>
class Button
{
private:
	std::function<void(Args...)> callback;
	Rect rect;
public:
	Button(const Rect &rect, const std::function<void(Args...)> &callback)
		: callback(callback), rect(rect) { }

	Button(int x, int y, int width, int height, const std::function<void(Args...)> &callback)
		: callback(callback), rect(x, y, width, height) { }

	Button(const Int2 &center, int width, int height, const std::function<void(Args...)> &callback)
		: Button(center.x - (width / 2), center.y - (height / 2), width, height, callback) { }

	// "Hidden" button, intended only as a hotkey.
	Button(const std::function<void(Args...)> &callback)
		: Button(0, 0, 0, 0, callback) { }

	// Empty button with no dimensions or callback, to be set later.
	Button()
		: Button(0, 0, 0, 0, std::function<void(Args...)>()) { }

	Button(Button&&) = default;

	Button &operator=(Button&&) = default;

	const Rect &getRect() const
	{
		return this->rect;
	}

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
		return this->rect.width;
	}

	int getHeight() const
	{
		return this->rect.height;
	}

	// Returns whether the button's area contains the given point.
	bool contains(const Int2 &point) const
	{
		return this->rect.contains(point);
	}

	void setX(int x)
	{
		this->rect.x = x;
	}

	void setY(int y)
	{
		this->rect.y = y;
	}

	void setWidth(int width)
	{
		this->rect.width = width;
	}

	void setHeight(int height)
	{
		this->rect.height = height;
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

// Allows the input manager to iterate over UI buttons and determine which one is clicked without
// worrying about buttons' variadic templates.
struct ButtonProxy
{
	using RectFunction = std::function<Rect()>;
	using Callback = std::function<void()>;
	using ActiveFunction = std::function<bool()>;

	MouseButtonType buttonType; // Which mouse button triggers a click.
	RectFunction rectFunc; // Classic UI space rect for clickable button. Might move around due to being e.g. a ListBox item.
	Rect parentRect; // Classic UI space rect that mouse clicks have to be within.
	Callback callback; // Called if the button is clicked.
	ActiveFunction isActiveFunc; // Contains a callable function if it can be inactive.

	ButtonProxy(MouseButtonType buttonType, const RectFunction &rectFunc, const Callback &callback,
		const Rect &parentRect = Rect(), const ActiveFunction &isActiveFunc = ActiveFunction());
	ButtonProxy();
};

#endif
