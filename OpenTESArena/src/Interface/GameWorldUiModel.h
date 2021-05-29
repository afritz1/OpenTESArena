#ifndef GAME_WORLD_UI_MODEL_H
#define GAME_WORLD_UI_MODEL_H

#include <string>

class Game;

namespace GameWorldUiModel
{
	std::string getPlayerNameText(Game &game);

	std::string getStatusButtonText(Game &game);
}

#endif
