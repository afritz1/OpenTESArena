#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceSearchSubPanel.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"

#include "components/utilities/String.h"

void ProvinceMapUiController::onSearchButtonSelected(Game &game, ProvinceMapPanel &panel, int provinceID)
{
	// Push text entry sub-panel for location searching.
	game.pushSubPanel<ProvinceSearchSubPanel>(panel, provinceID);
}

void ProvinceMapUiController::onTravelButtonSelected(Game &game, ProvinceMapPanel &panel)
{
	const auto &gameState = game.getGameState();
	const bool hasTravelData = gameState.getTravelData() != nullptr;

	if (hasTravelData)
	{
		// Fast travel to the selected destination.
		panel.handleFastTravel();
	}
	else
	{
		// Display error message about no selected destination.
		const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const std::string errorText = [&exeData]()
		{
			std::string text = exeData.travel.noDestination;

			// Remove carriage return at end.
			text.pop_back();

			// Replace carriage returns with newlines.
			text = String::replace(text, '\r', '\n');

			return text;
		}();

		std::unique_ptr<Panel> textPopUp = ProvinceMapUiModel::makeTextPopUp(game, errorText);
		game.pushSubPanel(std::move(textPopUp));
	}
}

void ProvinceMapUiController::onBackToWorldMapButtonSelected(Game &game)
{
	game.setPanel<WorldMapPanel>();
}

void ProvinceMapUiController::onTextPopUpSelected(Game &game)
{
	game.popSubPanel();
}

void ProvinceSearchUiController::onTextAccepted(Game &game, ProvinceSearchSubPanel &panel)
{
	auto &inputManager = game.getInputManager();
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
		panel.provinceMapPanel->trySelectLocation(*exactLocationIndex);

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
	panel.provinceMapPanel->trySelectLocation(locationID);

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
