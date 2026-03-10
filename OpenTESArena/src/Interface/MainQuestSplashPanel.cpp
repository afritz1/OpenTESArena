#include "MainQuestSplashPanel.h"
#include "MainQuestSplashUiState.h"
#include "../Game/Game.h"

MainQuestSplashPanel::MainQuestSplashPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(MainQuestSplashUI::ContextName, game);
}

MainQuestSplashPanel::~MainQuestSplashPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(MainQuestSplashUI::ContextName, game);
}

bool MainQuestSplashPanel::init()
{
	return true;
}
