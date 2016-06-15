#ifndef TOGGLE_BUTTON_H
#define TOGGLE_BUTTON_H

#include "Surface.h"

// A button for toggling between on and off. There is no change to the underlying
// surface when toggled; it's the caller's job to render differently.

// This doesn't call a function when toggled. That's the caller's job, for now.

class ToggleButton : public Surface
{
private:
	bool on;
public:
	// Default toggle button constructor.
	ToggleButton(int x, int y, int width, int height, bool on);

	// Toggle button constructor set to off.
	ToggleButton(int x, int y, int width, int height);

	// Centered toggle button constructor.
	ToggleButton(const Int2 &center, int width, int height, bool on);

	// Centered toggle button constructor set to off.
	ToggleButton(const Int2 &center, int width, int height);

	virtual ~ToggleButton();

	bool isOn() const;

	void toggle();
};

#endif
