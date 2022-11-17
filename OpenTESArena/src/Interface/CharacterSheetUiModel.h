#ifndef CHARACTER_SHEET_UI_MODEL_H
#define CHARACTER_SHEET_UI_MODEL_H

#include <string>
#include <vector>

#include "../Entities/PrimaryAttribute.h"

class Game;

namespace CharacterSheetUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game& game);
	std::vector<PrimaryAttribute> getPlayerAttributes(Game& game);
}

#endif
