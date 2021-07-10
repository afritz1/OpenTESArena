#include "InputActionEvents.h"

InputActionCallbackValues::InputActionCallbackValues()
{
	this->performed = false;
	this->held = false;
	this->released = false;
}

void InputActionCallbackValues::init(bool performed, bool held, bool released)
{
	this->performed = performed;
	this->held = held;
	this->released = released;
}
