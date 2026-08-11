#include "EntityGeneration.h"

EntityGenInfo::EntityGenInfo()
{
	this->playerLevel = 0;
	this->nightLightsAreActive = false;
	this->cityType = static_cast<ArenaCityType>(-1);
	this->interiorType = static_cast<ArenaInteriorType>(-1);
	this->interiorLevelIndex = -1;
	this->provinceID = -1;
}

void EntityGenInfo::init(int playerLevel, bool nightLightsAreActive, ArenaCityType cityType, ArenaInteriorType interiorType, int interiorLevelIndex, int provinceID)
{
	this->playerLevel = playerLevel;
	this->nightLightsAreActive = nightLightsAreActive;
	this->cityType = cityType;
	this->interiorType = interiorType;
	this->interiorLevelIndex = interiorLevelIndex;
	this->provinceID = provinceID;
}
