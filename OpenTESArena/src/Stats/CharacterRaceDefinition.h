#pragma once

#include <string>

#include "../Assets/TextureAsset.h"
#include "../WorldMap/ProvinceLibrary.h"

struct CharacterRaceDefinition
{
	ProvinceDefinitionID provinceDefID;
	int originalRaceIndex;
	char singularName[32];
	char pluralName[32];
	double swimmingMoveSpeed;
	double swimmingStaminaLossMultiplier;
	double climbingSpeedScale;
	TextureAsset maleCharSheetBodyTextureAsset;
	std::string maleCharSheetHeadsFilename;
	std::string maleGameUiHeadsFilename;
	TextureAsset femaleCharSheetBodyTextureAsset;
	std::string femaleCharSheetHeadsFilename;
	std::string femaleGameUiHeadsFilename;

	CharacterRaceDefinition();

	void init(ProvinceDefinitionID provinceDefID, int originalRaceIndex, const char *singularName, const char *pluralName, double swimmingMoveSpeed,
		double swimmingStaminaLossMultiplier, double climbingSpeedScale, const TextureAsset &maleCharSheetBodyTextureAsset,
		const std::string &maleCharSheetHeadsFilename, const std::string &maleGameUiHeadsFilename, const TextureAsset &femaleCharSheetBodyTextureAsset,
		const std::string &femaleCharSheetHeadsFilename, const std::string &femaleGameUiHeadsFilename);
};
