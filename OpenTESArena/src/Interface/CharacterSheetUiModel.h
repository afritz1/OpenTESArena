#ifndef CHARACTER_SHEET_UI_MODEL_H
#define CHARACTER_SHEET_UI_MODEL_H

#include <string>
#include <vector>

#include "../Stats/PrimaryAttribute.h"

class Game;

namespace CharacterSheetUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	const PrimaryAttributes &getPlayerAttributes(Game &game);
	std::string getPlayerExperience(Game &game);
	std::string getPlayerLevel(Game &game);
}

#endif
