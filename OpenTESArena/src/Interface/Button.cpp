#include <cassert>

#include "Button.h"

#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"

Button::Button(int x, int y, int width, int height, 
	const std::function<void(GameState*)> &function)
{
	this->function = function;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}

Button::Button(const Int2 &center, int width, int height, 
	const std::function<void(GameState*)> &function)
	: Button(center.getX() - (width / 2), center.getY() - (height / 2), 
		width, height, function) { }

Button::Button(const std::function<void(GameState*)> &function)
	: Button(0, 0, 1, 1, function) { }

Button::~Button()
{

}

bool Button::contains(const Int2 &point)
{
	Rect rect(this->x, this->y, this->width, this->height);
	return rect.contains(point);
}

void Button::click(GameState *gameState)
{
	this->function(gameState);
}
