#include "GameWorldPanel.h"
#include "GameWorldUiState.h"
#include "../Game/Game.h"

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(GameWorldUI::ContextName, game);
}

GameWorldPanel::~GameWorldPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(GameWorldUI::ContextName, game);
}

bool GameWorldPanel::init()
{
	return true;
}

void GameWorldPanel::onPauseChanged(bool paused)
{
	Panel::onPauseChanged(paused);

	Game &game = this->getGame();
	const Options &options = game.options;
	if (options.getGraphics_ModernInterface())
	{
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}

	game.shouldSimulateScene = !paused;
}
