#pragma once

#include <string>

#include "LocationDefinition.h"

struct LocationInstance
{
	std::string nameOverride; // Useful for quest dungeons.
	int locationDefIndex; // Index in province location definitions.
	bool visible;

	void init(int locationDefIndex, const LocationDefinition &locationDef);

	// Gets the location instance's name if it overrides its location definition's,
	// otherwise defaults to the location definition's.
	const std::string &getName(const LocationDefinition &locationDef) const;
};
