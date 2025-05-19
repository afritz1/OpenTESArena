#ifndef CHARACTER_SHEET_UI_MODEL_H
#define CHARACTER_SHEET_UI_MODEL_H

#include <string>
#include <vector>

#include "../Stats/PrimaryAttribute.h"

class Game;

namespace CharacterSheetUiModel
{
	std::string getDerivedAttributeDisplayString(int value);

	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	const PrimaryAttributes &getPlayerAttributes(Game &game);
	std::string getPlayerExperience(Game &game);
	std::string getPlayerLevel(Game &game);
	std::string getPlayerHealth(Game &game);
	std::string getPlayerStamina(Game &game);
	std::string getPlayerSpellPoints(Game &game);
	std::string getPlayerBonusDamage(Game &game);
	std::string getPlayerMaxWeight(Game &game);
	std::string getPlayerMagicDefense(Game &game);
	std::string getPlayerBonusToHit(Game &game);
	std::string getPlayerBonusToDefend(Game &game);
	std::string getPlayerBonusToHealth(Game &game);
	std::string getPlayerHealMod(Game &game);
	std::string getPlayerCharisma(Game &game);
	std::string getPlayerGold(Game &game);
}

#endif
