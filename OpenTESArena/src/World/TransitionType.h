#ifndef TRANSITION_TYPE_H
#define TRANSITION_TYPE_H

enum class TransitionType
{
	CityGate, // Swaps to city or wilderness, whichever is inactive.
	EnterInterior,
	ExitInterior,
	LevelChange
};

#endif
