#ifndef WORLD_MAP_UI_CONTROLLER_H
#define WORLD_MAP_UI_CONTROLLER_H

class Game;

namespace WorldMapUiController
{
	// -- World map --

	void onBackToGameButtonSelected(Game &game);
	void onProvinceButtonSelected(Game &game, int provinceID);

	// -- Fast travel --

	void onFastTravelAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
	void onFastTravelCityArrivalPopUpSelected(Game &game);
}

#endif
