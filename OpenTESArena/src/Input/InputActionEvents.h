#ifndef INPUT_ACTION_EVENTS_H
#define INPUT_ACTION_EVENTS_H

#include <functional>

class Game;

struct InputActionCallbackValues
{
	Game &game;
	bool performed;
	bool held;
	bool released;

	InputActionCallbackValues(Game &game, bool performed, bool held, bool released);
};

using InputActionCallback = std::function<void(const InputActionCallbackValues &values)>;

#endif
