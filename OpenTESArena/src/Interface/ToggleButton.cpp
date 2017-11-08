#include "ToggleButton.h"
#include "../Math/Rect.h"

ToggleButton::ToggleButton(int x, int y, int width, int height, bool on, 
	const std::function<void(Game*)> &onFunction, 
	const std::function<void(Game*)> &offFunction)
	: onFunction(onFunction), offFunction(offFunction)
{
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->on = on;
}

ToggleButton::ToggleButton(const Int2 &center, int width, int height, bool on,
	const std::function<void(Game*)> &onFunction,
	const std::function<void(Game*)> &offFunction)
	: ToggleButton(center.x - (width / 2), center.y - (height / 2),
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

void ToggleButton::toggle(Game *game)
{
	this->on = !this->on;

	if (this->on)
	{
		this->onFunction(game);
	}
	else
	{
		this->offFunction(game);
	}
}
