#ifndef PROVINCE_MAP_UI_CONTROLLER_H
#define PROVINCE_MAP_UI_CONTROLLER_H

#include "ProvinceMapUiModel.h"

class Game;
class ProvinceMapPanel;

namespace ProvinceMapUiController
{
	void onSearchButtonSelected(Game &game, ProvinceMapPanel &panel, int provinceID);
	void onTravelButtonSelected(Game &game, ProvinceMapPanel &panel, bool hasTravelData);
	void onBackToWorldMapButtonSelected(Game &game, std::unique_ptr<ProvinceMapUiModel::TravelData> travelData);

	void onTextPopUpSelected(Game &game);
}

#endif
