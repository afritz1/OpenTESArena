#include "OptionsPanel.h"
#include "OptionsUiController.h"
#include "PauseMenuPanel.h"
#include "../Game/Game.h"

void OptionsUiController::onBackToPauseMenuButtonSelected(Game &game)
{
	game.setPanel<PauseMenuPanel>(game);
}

void OptionsUiController::onTabButtonSelected(OptionsPanel &panel, OptionsUiModel::Tab *currentTab,
	OptionsUiModel::Tab newTab)
{
	// Update display if the tab values are different.
	const bool tabsAreEqual = *currentTab == newTab;

	if (!tabsAreEqual)
	{
		*currentTab = newTab;
		panel.updateVisibleOptionTextBoxes();
	}
}
