#include "InputActionEvents.h"

InputActionCallbackValues::InputActionCallbackValues(Game &game, bool performed, bool held, bool released)
	: game(game)
{
	this->performed = performed;
	this->held = held;
	this->released = released;
}
