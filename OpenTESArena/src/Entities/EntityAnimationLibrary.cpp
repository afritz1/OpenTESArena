#include <algorithm>

#include "EntityAnimationLibrary.h"

#include "components/debug/Debug.h"

bool EntityAnimationLibrary::tryGetAnimDefIndex(const std::string &animName, int *outIndex) const
{
	const auto iter = std::find_if(this->animDefs.begin(), this->animDefs.end(),
		[&animName](const EntityAnimationDefinition &def)
	{
		return def.getName() == animName;
	});

	if (iter != this->animDefs.end())
	{
		*outIndex = static_cast<int>(std::distance(this->animDefs.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

void EntityAnimationLibrary::addAnimDef(EntityAnimationDefinition &&animDef)
{
	// Check for duplicates.
	int index;
	if (this->tryGetAnimDefIndex(animDef.getName(), &index))
	{
		DebugLogWarning("Entity animation \"" + animDef.getName() + "\" already registered.");
		return;
	}

	this->animDefs.push_back(std::move(animDef));
}

void EntityAnimationLibrary::clear()
{
	this->animDefs.clear();
}
