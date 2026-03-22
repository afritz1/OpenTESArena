#ifndef WORLD_MAP_UI_CONTROLLER_H
#define WORLD_MAP_UI_CONTROLLER_H

class Game;

namespace FastTravelUiController
{
	void onAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
}

#endif
