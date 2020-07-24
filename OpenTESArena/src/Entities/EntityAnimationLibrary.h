#ifndef ENTITY_ANIMATION_LIBRARY_H
#define ENTITY_ANIMATION_LIBRARY_H

#include <vector>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"

#include "components/utilities/BufferRef.h"

// Buffer reference wrapper for entity animation definitions to avoid dangling pointers.
using EntityAnimRef = BufferRef<const std::vector<EntityAnimationDefinition>, const EntityAnimationDefinition>;

class EntityAnimationLibrary
{
private:
	std::vector<EntityAnimationDefinition> animDefs;
public:
	static constexpr EntityAnimID NO_ID = -1;

	// Returns the ID of an animation if a mapping exists.
	bool tryGetAnimID(const char *animName, EntityAnimID *outID) const;

	// Returns animation reference wrapper to avoid dangling pointer.
	EntityAnimRef getAnimRef(EntityAnimID animID) const;

	// Returns raw animation reference, does not protect against dangling pointers.
	const EntityAnimationDefinition &getAnimHandle(EntityAnimID animID) const;

	bool tryAddAnimDef(EntityAnimationDefinition &&animDef, EntityAnimID *outID);
};

#endif
