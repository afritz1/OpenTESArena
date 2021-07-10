#ifndef INPUT_ACTION_MAP_H
#define INPUT_ACTION_MAP_H

#include <vector>

#include "InputActionDefinition.h"

struct InputActionMap
{
	std::vector<InputActionDefinition> defs;
	bool active;

	InputActionMap();
};

#endif
