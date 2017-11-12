#ifndef TOGGLE_BUTTON_H
#define TOGGLE_BUTTON_H

#include <functional>

#include "../Math/Vector2.h"

// A button for toggling on and off. When the button is toggled, the 
// function for the new toggle state is called.

// I wonder if this class should be redesigned to take variadic template arguments 
// like the new Button class.

class Game;

class ToggleButton
{
private:
	std::function<void(Game&)> onFunction;
	std::function<void(Game&)> offFunction;
	int x, y, width, height;
	bool on;
public:
	ToggleButton(int x, int y, int width, int height, bool on,
		const std::function<void(Game&)> &onFunction,
		const std::function<void(Game&)> &offFunction);
	ToggleButton(const Int2 &center, int width, int height, bool on,
		const std::function<void(Game&)> &onFunction,
		const std::function<void(Game&)> &offFunction);
	virtual ~ToggleButton();

	// Returns whether the button is toggled on.
	bool isOn() const;

	// Returns whether the button's area contains the given point.
	bool contains(const Int2 &point);

	// Switches the toggle state of the button.
	void toggle(Game &game);
};

#endif
