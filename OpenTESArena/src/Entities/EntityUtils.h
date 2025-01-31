#ifndef ENTITY_UTILS_H
#define ENTITY_UTILS_H

#include <optional>
#include <string>

#include "EntityDefinition.h"
#include "../World/Coord.h"

// Entity definition handle.
using EntityDefID = int;

class CharacterClassLibrary;
class EntityDefinitionLibrary;
class Random;

namespace EntityUtils
{
	bool isDynamicEntity(EntityDefinitionType defType);

	// Gets the display name of the entity definition type for debugging.
	std::string defTypeToString(const EntityDefinition &entityDef);

	// Returns whether the given entity definition ID is from a level, or if it is in the
	// entity definition library.
	bool isLevelDependentDef(EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary);

	// Returns whether the given entity definition is for a streetlight. Note that wilderness streetlights
	// do not have their activation state updated in the original game like city streetlights do.
	bool isStreetlight(const EntityDefinition &entityDef);

	bool isGhost(const EntityDefinition &entityDef);
	bool isPuddle(const EntityDefinition &entityDef);

	int getYOffset(const EntityDefinition &entityDef);

	bool hasCollision(const EntityDefinition &entityDef);

	// Returns the entity definition's light radius, if any.
	std::optional<double> tryGetLightRadius(const EntityDefinition &entityDef);

	// Gets the max width and height from the entity animation's frames.
	void getAnimationMaxDims(const EntityAnimationDefinition &animDef, double *outMaxWidth, double *outMaxHeight);

	// Gets the world space Y position in the center of the entity
	double getCenterY(double feetY, double bboxHeight);

	// Returns whether the entity definition has a display name.
	bool tryGetDisplayName(const EntityDefinition &entityDef,
		const CharacterClassLibrary &charClassLibrary, std::string *outName);

	// Arbitrary value for how far away a creature can be heard from.
	// @todo: make this be part of the player, not creatures.
	constexpr double HearingDistance = 6.0;

	bool withinHearingDistance(const CoordDouble3 &listenerCoord, const CoordDouble2 &soundCoord, double ceilingScale);

	double nextCreatureSoundWaitSeconds(Random &random);
}

#endif
