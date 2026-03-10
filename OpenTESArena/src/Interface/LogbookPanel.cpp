#include "LogbookPanel.h"
#include "LogbookUiState.h"
#include "../Game/Game.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(LogbookUI::ContextName, game);
}

LogbookPanel::~LogbookPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(LogbookUI::ContextName, game);
}

bool LogbookPanel::init()
{
	return true;
}
