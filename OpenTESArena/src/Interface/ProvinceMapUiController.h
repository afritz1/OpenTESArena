#ifndef PROVINCE_MAP_UI_CONTROLLER_H
#define PROVINCE_MAP_UI_CONTROLLER_H

#include "ProvinceMapUiModel.h"

class Game;
class ListBox;
class ProvinceMapPanel;
class ProvinceSearchSubPanel;

namespace ProvinceMapUiController
{
	// -- Province panel --

	void onSearchButtonSelected(Game &game, ProvinceMapPanel &panel, int provinceID);
	void onTravelButtonSelected(Game &game, ProvinceMapPanel &panel, bool hasTravelData);
	void onBackToWorldMapButtonSelected(Game &game, std::unique_ptr<ProvinceMapUiModel::TravelData> travelData);

	void onTextPopUpSelected(Game &game);

	// -- Search sub-panel --

	void onSearchTextAccepted(Game &game, ProvinceSearchSubPanel &panel);
	void onSearchListLocationSelected(Game &game, ProvinceSearchSubPanel &panel, int locationID);
	void onSearchListUpButtonSelected(ListBox &listBox);
	void onSearchListDownButtonSelected(ListBox &listBox);
}

#endif
