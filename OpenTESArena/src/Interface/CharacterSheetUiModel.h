#ifndef CHARACTER_SHEET_UI_MODEL_H
#define CHARACTER_SHEET_UI_MODEL_H

#include <string>

class Game;

namespace CharacterSheetUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game& game);
	std::string getPlayerStrengthText(Game& game);
	std::string getPlayerIntelligenceText(Game& game);
	std::string getPlayerWillpowerText(Game& game);
	std::string getPlayerAgilityText(Game& game);
	std::string getPlayerSpeedText(Game& game);
	std::string getPlayerEnduranceText(Game& game);
	std::string getPlayerPersonalityText(Game& game);
	std::string getPlayerLuckText(Game &game);
}

#endif
