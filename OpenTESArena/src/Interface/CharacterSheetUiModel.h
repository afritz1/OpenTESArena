#ifndef CHARACTER_SHEET_UI_MODEL_H
#define CHARACTER_SHEET_UI_MODEL_H

#include <string>

#include "../Stats/PrimaryAttribute.h"

class Game;

namespace CharacterSheetUiModel
{
	// For UI elements.
	constexpr const char *DerivedAttributeUiNames[] =
	{
		"BonusDamage",
		"MaxWeight",
		"MagicDefense",
		"BonusToHit",
		"BonusToDefend",
		"BonusToHealth",
		"HealMod",
		"BonusToCharisma",
	};

	std::string getStatusValueCurrentAndMaxString(double currentValue, double maxValue);
	std::string getDerivedAttributeDisplayString(int value);

	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	DerivedAttributes getPlayerDerivedAttributes(Game &game);
	std::string getPlayerExperience(Game &game);
	std::string getPlayerLevel(Game &game);
	std::string getPlayerHealth(Game &game);
	std::string getPlayerStamina(Game &game);
	std::string getPlayerSpellPoints(Game &game);
	std::string getPlayerGold(Game &game);
}

#endif
