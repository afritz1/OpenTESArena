#include "OptionsPanel.h"
#include "OptionsUiState.h"
#include "../Game/Game.h"

OptionsPanel::OptionsPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(OptionsUI::ContextName, game);
}

OptionsPanel::~OptionsPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(OptionsUI::ContextName, game);
}

bool OptionsPanel::init()
{
	return true;
}
