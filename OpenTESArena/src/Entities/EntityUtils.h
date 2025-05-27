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

	// Whether system resources (a.k.a. textures) for this entity type are loaded/unloaded per scene.
	// Should be false for entities that are often in every scene (e.g. VFX).
	bool isSceneManagedResource(EntityDefinitionType entityDefType);

	int getYOffset(const EntityDefinition &entityDef);

	bool hasCollision(const EntityDefinition &entityDef);
	bool canDie(const EntityDefinition &entityDef);
	std::optional<int> tryGetDeathAnimStateIndex(const EntityAnimationDefinition &animDef);
	bool leavesCorpse(const EntityDefinition &entityDef);

	// Returns the entity definition's light radius, if any.
	std::optional<double> tryGetLightRadius(const EntityDefinition &entityDef);

	// Gets the max width and height from the entity animation's frames.
	void getAnimationMaxDims(const EntityAnimationDefinition &animDef, double *outMaxWidth, double *outMaxHeight);

	// Returns whether the entity definition has a display name.
	bool tryGetDisplayName(const EntityDefinition &entityDef,
		const CharacterClassLibrary &charClassLibrary, std::string *outName);

	// Arbitrary value for how far away a creature can be heard from.
	// @todo: make this be part of the player, not creatures.
	constexpr double HearingDistance = 6.0;

	bool withinHearingDistance(const WorldDouble3 &listenerPosition, const WorldDouble3 &soundPosition);

	double nextCreatureSoundWaitSeconds(Random &random);
}

#endif
