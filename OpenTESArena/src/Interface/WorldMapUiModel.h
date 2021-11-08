#ifndef WORLD_MAP_UI_MODEL_H
#define WORLD_MAP_UI_MODEL_H

#include <array>
#include <memory>
#include <optional>
#include <string>

#include "../Math/Vector2.h"

class Game;
class Panel;
class WorldMapMask;

namespace WorldMapUiModel
{
	static constexpr int EXIT_BUTTON_MASK_ID = 9;
	static constexpr int MASK_COUNT = EXIT_BUTTON_MASK_ID + 1;

	std::string getProvinceNameOffsetFilename();

	// Gets the mask click area for a province or the exit button.
	const WorldMapMask &getMask(const Game &game, int maskID);

	// Gets the province ID or exit button ID of the hovered pixel on the world map.
	std::optional<int> getMaskID(Game &game, const Int2 &mousePosition, bool ignoreCenterProvince,
		bool ignoreExitButton);
}

namespace FastTravelUiModel
{
	// Shortest amount of time the fast travel animation can show for.
	static constexpr double AnimationMinSeconds = 1.0;

	// Advances the game clock after having fast travelled.
	void tickTravelTime(Game &game, int travelDays);

	std::string getCityArrivalMessage(Game &game, int targetProvinceID, int targetLocationID, int travelDays);

	// Creates a text sub-panel for display when the player arrives at a city.
	// - @todo: holiday pop-up function.
	std::unique_ptr<Panel> makeCityArrivalPopUp(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
}

#endif
