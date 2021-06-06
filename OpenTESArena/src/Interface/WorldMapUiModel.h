#ifndef WORLD_MAP_UI_MODEL_H
#define WORLD_MAP_UI_MODEL_H

#include <array>
#include <memory>
#include <string>

#include "../Math/Vector2.h"

class Game;
class Panel;

namespace WorldMapUiModel
{
	// -- World map --

	std::string getProvinceNameOffsetFilename();

	// -- Fast travel --

	// Shortest amount of time the fast travel animation can show for.
	static constexpr double FastTravelAnimationMinSeconds = 1.0;

	// Advances the game clock after having fast travelled.
	void tickTravelTime(Game &game, int travelDays);

	std::string getCityArrivalMessage(Game &game, int targetProvinceID, int targetLocationID, int travelDays);

	// Creates a text sub-panel for display when the player arrives at a city.
	// - @todo: holiday pop-up function.
	std::unique_ptr<Panel> makeCityArrivalPopUp(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
}

#endif
