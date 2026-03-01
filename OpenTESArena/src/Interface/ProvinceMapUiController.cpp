#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceSearchSubPanel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"

#include "components/utilities/String.h"

void ProvinceMapUiController::onTextPopUpSelected(Game &game)
{
	game.popSubPanel();
}

void ProvinceSearchUiController::onTextAccepted(Game &game, ProvinceSearchSubPanel &panel)
{
	auto &inputManager = game.inputManager;
	inputManager.setTextInputMode(false);

	// Determine what to do with the current location name. If it is a valid match
	// with one of the visible locations in the province, then select that location.
	// Otherwise, display the list box of locations sorted by their location index.
	const int *exactLocationIndex = nullptr;
	panel.locationsListIndices = ProvinceSearchUiModel::getMatchingLocations(game,
		panel.locationName, panel.provinceID, &exactLocationIndex);

	if (exactLocationIndex != nullptr)
	{
		// The location name is an exact match. Try to select the location in the province
		// map panel based on whether the player is already there.
		//panel.provinceMapPanel->trySelectLocation(*exactLocationIndex);
		DebugNotImplemented();

		// Return to the province map panel.
		game.popSubPanel();
	}
	else
	{
		// No exact match. Change to list mode.
		panel.initLocationsList();
		panel.mode = ProvinceSearchUiModel::Mode::List;
	}
}

void ProvinceSearchUiController::onListLocationSelected(Game &game, ProvinceSearchSubPanel &panel, int locationID)
{
	// Try to select the location in the province map panel based on whether the
	// player is already there.
	//panel.provinceMapPanel->trySelectLocation(locationID);
	DebugNotImplemented();

	// Return to the province map panel.
	game.popSubPanel();
}

void ProvinceSearchUiController::onListUpButtonSelected(ListBox &listBox)
{
	listBox.scrollUp();
}

void ProvinceSearchUiController::onListDownButtonSelected(ListBox &listBox)
{
	listBox.scrollDown();
}
