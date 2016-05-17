#include <cassert>

#include "Button.h"

#include "../Math/Int2.h"

Button::Button(int x, int y, int width, int height, const std::function<void()> &function)
	: Surface(x, y, width, height)
{
	this->function = function;
}

Button::Button(const Int2 &center, int width, int height, const std::function<void()>& function)
	: Button(center.getX() - (width / 2), center.getY() - (height / 2), width, height, function) { }

Button::Button(const std::function<void()> &function)
	: Button(0, 0, 1, 1, function) { }

Button::~Button()
{

}

void Button::click()
{
	this->function();
}
