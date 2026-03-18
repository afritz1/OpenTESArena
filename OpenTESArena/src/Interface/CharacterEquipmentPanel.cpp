#include "CharacterEquipmentPanel.h"
#include "CharacterEquipmentUiState.h"
#include "../Game/Game.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(CharacterEquipmentUI::ContextName, game);
}

CharacterEquipmentPanel::~CharacterEquipmentPanel()
{
	Game &game = this->getGame();
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(CharacterEquipmentUI::ContextName, game);
}

bool CharacterEquipmentPanel::init()
{
	return true;
}
