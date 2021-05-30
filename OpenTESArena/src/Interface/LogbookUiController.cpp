#include "GameWorldPanel.h"
#include "LogbookUiController.h"
#include "../Game/Game.h"

void LogbookUiController::onBackButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>(game);
}
