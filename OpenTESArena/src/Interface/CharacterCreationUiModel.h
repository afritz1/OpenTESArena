#ifndef CHARACTER_CREATION_UI_MODEL_H
#define CHARACTER_CREATION_UI_MODEL_H

#include <string>

class Game;

namespace CharacterCreationUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);

	std::string getChooseAttributesText(Game &game);

	std::string getAttributesMessageBoxTitleText(Game &game);
	std::string getAttributesMessageBoxSaveText(Game &game);
	std::string getAttributesMessageBoxRerollText(Game &game);

	std::string getAppearanceMessageBoxText(Game &game);
}

#endif
