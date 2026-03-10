#include "LoadSavePanel.h"
#include "LoadSaveUiState.h"
#include "../Game/Game.h"

LoadSavePanel::LoadSavePanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(LoadSaveUI::ContextName, game);
}

LoadSavePanel::~LoadSavePanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(LoadSaveUI::ContextName, game);
}

bool LoadSavePanel::init()
{
	return true;
}
