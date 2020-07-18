#ifndef ENTITY_ANIMATION_LIBRARY_H
#define ENTITY_ANIMATION_LIBRARY_H

#include <string>
#include <vector>

#include "EntityAnimationDefinition.h"

class EntityAnimationLibrary
{
private:
	std::vector<EntityAnimationDefinition> animDefs;
public:
	bool tryGetAnimDefIndex(const std::string &animName, int *outIndex) const;

	void addAnimDef(EntityAnimationDefinition &&animDef);
	void clear();
};

#endif
