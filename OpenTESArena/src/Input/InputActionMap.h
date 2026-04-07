#pragma once

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
