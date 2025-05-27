#ifndef TRANSITION_TYPE_H
#define TRANSITION_TYPE_H

enum class TransitionType
{
	CityGate, // Swaps between city/wilderness.
	EnterInterior,
	ExitInterior,
	InteriorLevelChange
};

#endif
