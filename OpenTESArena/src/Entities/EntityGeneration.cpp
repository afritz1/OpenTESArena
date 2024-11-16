#include "EntityGeneration.h"

void EntityGeneration::EntityGenInfo::init(bool nightLightsAreActive)
{
	this->nightLightsAreActive = nightLightsAreActive;
}

const std::string &EntityGeneration::getDefaultAnimationStateName(const EntityDefinition &entityDef, const EntityGenInfo &genInfo)
{
	if (!EntityUtils::isStreetlight(entityDef))
	{
		return EntityAnimationUtils::STATE_IDLE;
	}
	else
	{
		return genInfo.nightLightsAreActive ? EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;
	}
}
