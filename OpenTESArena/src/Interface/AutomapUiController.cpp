#include "AutomapUiController.h"
#include "GameWorldPanel.h"
#include "../Game/Game.h"

void AutomapUiController::onBackToGameButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>();
}
