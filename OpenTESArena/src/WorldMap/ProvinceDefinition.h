#pragma once

#include <functional>
#include <vector>

#include "LocationDefinition.h"
#include "../Math/Rect.h"

class BinaryAssetLibrary;

using ProvinceLocationPredicate = std::function<bool(const LocationDefinition&)>;

struct ProvinceDefinition
{
	std::vector<LocationDefinition> locations;
	std::string name;
	int globalX, globalY, globalW, globalH; // Province-to-world-map projection.
	int originalProvinceIndex;
	bool hasAnimatedDistantLand;

	// Initialize from original game data.
	void init(int originalProvinceIndex, const BinaryAssetLibrary &binaryAssetLibrary);
	// @todo: eventually have init(const char *filename) for custom provinces.

	Rect getGlobalRect() const;

	// Returns whether the two definitions reference the same province in the world map.
	bool matches(const ProvinceDefinition &other) const;

	int findLocationIndexIf(const ProvinceLocationPredicate &predicate) const;
};
