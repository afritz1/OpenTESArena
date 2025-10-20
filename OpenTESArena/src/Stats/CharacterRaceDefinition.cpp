#include <algorithm>
#include <cstring>

#include "CharacterRaceDefinition.h"

#include "components/debug/Debug.h"

CharacterRaceDefinition::CharacterRaceDefinition()
{
	this->provinceID = -1;
	std::fill(std::begin(this->singularName), std::end(this->singularName), '\0');
	std::fill(std::begin(this->pluralName), std::end(this->pluralName), '\0');
	this->swimmingMoveSpeed = 0.0;
	this->swimmingStaminaLossMultiplier = 0.0;
	this->climbingSpeedScale = 0.0;
}

void CharacterRaceDefinition::init(int provinceID, const char *singularName, const char *pluralName, double swimmingMoveSpeed, double swimmingStaminaLossMultiplier, double climbingSpeedScale,
	const TextureAsset &maleCharSheetBodyTextureAsset, const std::string &maleCharSheetHeadsFilename, const std::string &maleGameUiHeadsFilename,
	const TextureAsset &femaleCharSheetBodyTextureAsset, const std::string &femaleCharSheetHeadsFilename, const std::string &femaleGameUiHeadsFilename)
{
	DebugAssert(provinceID >= 0);
	this->provinceID = provinceID;
	std::snprintf(std::begin(this->singularName), std::size(this->singularName), "%s", singularName);
	std::snprintf(std::begin(this->pluralName), std::size(this->pluralName), "%s", pluralName);
	this->swimmingMoveSpeed = swimmingMoveSpeed;
	this->swimmingStaminaLossMultiplier = swimmingStaminaLossMultiplier;
	this->climbingSpeedScale = climbingSpeedScale;
	this->maleCharSheetBodyTextureAsset = maleCharSheetBodyTextureAsset;
	this->maleCharSheetHeadsFilename = maleCharSheetHeadsFilename;
	this->maleGameUiHeadsFilename = maleGameUiHeadsFilename;
	this->femaleCharSheetBodyTextureAsset = femaleCharSheetBodyTextureAsset;
	this->femaleCharSheetHeadsFilename = femaleCharSheetHeadsFilename;
	this->femaleGameUiHeadsFilename = femaleGameUiHeadsFilename;
}
