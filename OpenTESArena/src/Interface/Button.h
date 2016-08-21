#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include "Surface.h"

// All buttons are given a pointer to the current game state.

class GameState;

class Button : public Surface
{
private:
	std::function<void(GameState*)> function;
public:
	Button(int32_t x, int32_t y, int32_t width, int32_t height, 
		const std::function<void(GameState*)> &function);
	Button(const Int2 &center, int32_t width, int32_t height,
		const std::function<void(GameState*)> &function);

	// "Hidden" button for hotkeys. It is not intended to be drawn anywhere.
	Button(const std::function<void(GameState*)> &function);

	virtual ~Button();

	void click(GameState *gameState);
};

#endif
