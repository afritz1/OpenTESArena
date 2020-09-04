#ifndef ENTITY_UTILS_H
#define ENTITY_UTILS_H

#include <string>

// Entity instance handle.
using EntityID = int;

// Entity definition handle.
using EntityDefID = int;

// Renderer handle. Can be shared between entity instances that look the same.
using EntityRenderID = int;

class CharacterClassLibrary;
class EntityDefinition;

namespace EntityUtils
{
	// Gets the display name of the entity definition type for debugging.
	std::string defTypeToString(const EntityDefinition &entityDef);

	int getYOffset(const EntityDefinition &entityDef);

	// Returns whether the entity definition has light intensity.
	bool tryGetLightIntensity(const EntityDefinition &entityDef, int *outIntensity);

	// Returns whether the entity definition has a display name.
	bool tryGetDisplayName(const EntityDefinition &entityDef,
		const CharacterClassLibrary &charClassLibrary, std::string *outName);
}

#endif
