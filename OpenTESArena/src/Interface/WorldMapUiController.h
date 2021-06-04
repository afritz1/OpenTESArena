#ifndef WORLD_MAP_UI_CONTROLLER_H
#define WORLD_MAP_UI_CONTROLLER_H

#include <memory>

#include "ProvinceMapUiModel.h"

class Game;

namespace WorldMapUiController
{
	// -- World map --

	void onBackToGameButtonSelected(Game &game);
	void onProvinceButtonSelected(Game &game, int provinceID, std::unique_ptr<ProvinceMapUiModel::TravelData> travelData);

	// -- Fast travel --

	void onFastTravelAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
	void onFastTravelCityArrivalPopUpSelected(Game &game);
}

#endif
