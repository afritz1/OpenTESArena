#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

#include "EntityAnimationLibrary.h"

#include "components/debug/Debug.h"

bool EntityAnimationLibrary::tryGetAnimID(const char *animName, EntityAnimID *outID) const
{
	if ((animName == nullptr) || (std::strlen(animName) == 0))
	{
		return false;
	}

	const std::string_view animNameView(animName);
	const auto iter = std::find_if(this->animDefs.begin(), this->animDefs.end(),
		[&animNameView](const EntityAnimationDefinition &def)
	{
		return def.getName() == animNameView;
	});

	if (iter != this->animDefs.end())
	{
		*outID = static_cast<EntityAnimID>(std::distance(this->animDefs.begin(), iter));
		return true;
	}
	else
	{
		return false;
	}
}

EntityAnimRef EntityAnimationLibrary::getAnimRef(EntityAnimID animID) const
{
	return EntityAnimRef(&this->animDefs, static_cast<int>(animID));
}

const EntityAnimationDefinition &EntityAnimationLibrary::getAnimHandle(EntityAnimID animID) const
{
	DebugAssertIndex(this->animDefs, animID);
	return this->animDefs[animID];
}

bool EntityAnimationLibrary::tryAddAnimDef(EntityAnimationDefinition &&animDef, EntityAnimID *outID)
{
	const std::string &animName = animDef.getName();
	if (animName.size() == 0)
	{
		return false;
	}

	// Duplicates not allowed.
	EntityAnimID animID;
	if (this->tryGetAnimID(animName.c_str(), &animID))
	{
		DebugLogWarning("Entity animation \"" + animName +
			"\" already registered (ID: " + std::to_string(animID) + ").");
		return false;
	}

	this->animDefs.push_back(std::move(animDef));
	*outID = static_cast<EntityAnimID>(this->animDefs.size()) - 1;
	return true;
}
