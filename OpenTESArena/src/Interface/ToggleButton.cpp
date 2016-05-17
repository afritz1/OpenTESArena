#include <cassert>

#include "ToggleButton.h"

#include "../Math/Int2.h"

ToggleButton::ToggleButton(int x, int y, int width, int height, bool on)
	: Surface(x, y, width, height)
{
	this->on = on;
}

ToggleButton::ToggleButton(int x, int y, int width, int height)
	: ToggleButton(x, y, width, height, false) { }

ToggleButton::ToggleButton(const Int2 &center, int width, int height, bool on)
	: ToggleButton(center.getX() - (width / 2), center.getY() - (height / 2),
		width, height, on) { }

ToggleButton::ToggleButton(const Int2 &center, int width, int height)
	: ToggleButton(center, width, height, false) { }

ToggleButton::~ToggleButton()
{

}

const bool &ToggleButton::isOn() const
{
	return this->on;
}

void ToggleButton::toggle()
{
	this->on = !this->isOn();
}
