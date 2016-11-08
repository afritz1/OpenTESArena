#include "ToggleButton.h"

#include "../Math/Int2.h"
#include "../Math/Rect.h"

ToggleButton::ToggleButton(int x, int y, int width, int height, bool on, 
	const std::function<void(GameState*)> &onFunction, 
	const std::function<void(GameState*)> &offFunction)
{
	this->onFunction = onFunction;
	this->offFunction = offFunction;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->on = on;
}

ToggleButton::ToggleButton(const Int2 &center, int width, int height, bool on,
	const std::function<void(GameState*)> &onFunction,
	const std::function<void(GameState*)> &offFunction)
	: ToggleButton(center.getX() - (width / 2), center.getY() - (height / 2),
		width, height, on, onFunction, offFunction) { }

ToggleButton::~ToggleButton()
{

}

bool ToggleButton::isOn() const
{
	return this->on;
}

bool ToggleButton::contains(const Int2 &point)
{
	Rect rect(this->x, this->y, this->width, this->height);
	return rect.contains(point);
}

void ToggleButton::toggle(GameState *gameState)
{
	this->on = !this->on;

	if (this->on)
	{
		this->onFunction(gameState);
	}
	else
	{
		this->offFunction(gameState);
	}
}
