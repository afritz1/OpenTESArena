#include "ChooseGenderPanel.h"
#include "ChooseGenderUiState.h"
#include "../Game/Game.h"

ChooseGenderPanel::ChooseGenderPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(ChooseGenderUI::ContextName, game);
}

ChooseGenderPanel::~ChooseGenderPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(ChooseGenderUI::ContextName, game);
}

bool ChooseGenderPanel::init()
{
	return true;
}
