#ifndef PAUSE_MENU_UI_MODEL_H
#define PAUSE_MENU_UI_MODEL_H

#include <string>

class Game;

namespace PauseMenuUiModel
{
	std::string getSoundVolumeText(Game &game);
	std::string getMusicVolumeText(Game &game);
	std::string getOptionsButtonText(Game &game);
}

#endif
