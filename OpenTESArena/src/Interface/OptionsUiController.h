#ifndef OPTIONS_UI_CONTROLLER_H
#define OPTIONS_UI_CONTROLLER_H

#include "OptionsUiModel.h"

class Game;
class OptionsPanel;

namespace OptionsUiController
{
	void onBackButtonSelected(Game &game);
	void onTabButtonSelected(OptionsPanel &panel, OptionsUiModel::Tab *currentTab, OptionsUiModel::Tab newTab);
}

#endif
