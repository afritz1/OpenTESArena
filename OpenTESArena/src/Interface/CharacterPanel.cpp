#include "CharacterPanel.h"
#include "CharacterUiState.h"
#include "../Game/Game.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(CharacterUI::ContextName, game);
}

CharacterPanel::~CharacterPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(CharacterUI::ContextName, game);
}

bool CharacterPanel::init()
{
	return true;
}
