#include "EntityGeneration.h"

void EntityGenInfo::init(bool nightLightsAreActive, ArenaCityType cityType, ArenaInteriorType interiorType, int interiorLevelIndex)
{
	this->nightLightsAreActive = nightLightsAreActive;
	this->cityType = cityType;
	this->interiorType = interiorType;
	this->interiorLevelIndex = interiorLevelIndex;
}
