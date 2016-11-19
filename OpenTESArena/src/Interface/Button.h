#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

// All buttons are given a pointer to the current game state.

class Game;
class Int2;

class Button
{
private:
	std::function<void(Game*)> function;
	int x, y, width, height;
public:
	Button(int x, int y, int width, int height, 
		const std::function<void(Game*)> &function);
	Button(const Int2 &center, int width, int height,
		const std::function<void(Game*)> &function);

	// "Hidden" button, intended only as a hotkey.
	Button(const std::function<void(Game*)> &function);

	virtual ~Button();

	// Returns whether the button's area contains the given point.
	bool contains(const Int2 &point);

	// Calls the button's function.
	void click(Game *game);
};

#endif
