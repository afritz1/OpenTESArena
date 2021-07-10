#ifndef INPUT_ACTION_EVENTS_H
#define INPUT_ACTION_EVENTS_H

#include <functional>

struct InputActionCallbackValues
{
	// More than one of these might be true, like for key inputs where it's considered held as soon as it's down.
	bool performed;
	bool held;
	bool released;

	InputActionCallbackValues();

	void init(bool performed, bool held, bool released);
};

using InputActionCallback = std::function<void(const InputActionCallbackValues &values)>;

#endif
