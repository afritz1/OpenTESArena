#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceSearchSubPanel.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"

#include "components/utilities/String.h"

void ProvinceMapUiController::onSearchButtonSelected(Game &game, ProvinceMapPanel &panel, int provinceID)
{
	// Push text entry sub-panel for location searching.
	game.pushSubPanel<ProvinceSearchSubPanel>(game, panel, provinceID);
}

void ProvinceMapUiController::onTravelButtonSelected(Game &game, ProvinceMapPanel &panel, bool hasTravelData)
{
	if (hasTravelData)
	{
		// Fast travel to the selected destination.
		panel.handleFastTravel();
	}
	else
	{
		// Display error message about no selected destination.
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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

void ProvinceMapUiController::onBackToWorldMapButtonSelected(Game &game,
	std::unique_ptr<ProvinceMapUiModel::TravelData> travelData)
{
	game.setPanel<WorldMapPanel>(game, std::move(travelData));
}

void ProvinceMapUiController::onTextPopUpSelected(Game &game)
{
	game.popSubPanel();
}
