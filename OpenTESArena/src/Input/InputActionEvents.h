#pragma once

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
