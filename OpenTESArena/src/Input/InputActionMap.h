#ifndef INPUT_ACTION_MAP_H
#define INPUT_ACTION_MAP_H

#include <vector>

#include "InputActionDefinition.h"

struct InputActionMap
{
	std::string name;
	std::vector<InputActionDefinition> defs;
	bool allowedDuringTextEntry;
	bool active;

	InputActionMap();

	void init(const std::string &name, bool allowedDuringTextEntry, bool active);

	static std::vector<InputActionMap> loadDefaultMaps();
};

#endif
