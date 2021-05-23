#include "CharacterEquipmentPanel.h"
#include "CharacterSheetUiController.h"
#include "GameWorldPanel.h"
#include "../Game/Game.h"

void CharacterSheetUiController::onDoneButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>(game);
}

void CharacterSheetUiController::onNextPageButtonSelected(Game &game)
{
	game.setPanel<CharacterEquipmentPanel>(game);
}
