#ifndef INPUT_STATE_TYPE_H
#define INPUT_STATE_TYPE_H

// State types for non-mono-state inputs like button presses. Mono-state inputs like mouse wheel scrolls do
// not have a state type.

enum class InputStateType
{
	BeginPerform, // The instant the physical input starts.
	Performing, // While the physical input is considered still happening.
	EndPerform // The instant the physical input stops completely.
};

#endif
