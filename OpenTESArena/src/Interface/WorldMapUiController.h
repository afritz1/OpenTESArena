#ifndef WORLD_MAP_UI_CONTROLLER_H
#define WORLD_MAP_UI_CONTROLLER_H

class Game;

namespace WorldMapUiController
{
	void onBackToGameButtonSelected(Game &game);
	void onProvinceButtonSelected(Game &game, int provinceID);
}

namespace FastTravelUiController
{
	void onAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
	void onCityArrivalPopUpSelected(Game &game);
}

#endif
