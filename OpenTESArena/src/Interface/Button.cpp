#include <cassert>

#include "Button.h"

#include "../Game/Game.h"
#include "../Math/Rect.h"

Button::Button(int x, int y, int width, int height, 
	const std::function<void(Game*)> &function)
{
	this->function = function;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}

Button::Button(const Int2 &center, int width, int height, 
	const std::function<void(Game*)> &function)
	: Button(center.x - (width / 2), center.y - (height / 2), 
		width, height, function) { }

Button::Button(const std::function<void(Game*)> &function)
	: Button(0, 0, 1, 1, function) { }

Button::~Button()
{

}

int Button::getX() const
{
	return this->x;
}

int Button::getY() const
{
	return this->y;
}

int Button::getWidth() const
{
	return this->width;
}

int Button::getHeight() const
{
	return this->height;
}

bool Button::contains(const Int2 &point)
{
	Rect rect(this->x, this->y, this->width, this->height);
	return rect.contains(point);
}

void Button::click(Game *game)
{
	this->function(game);
}
