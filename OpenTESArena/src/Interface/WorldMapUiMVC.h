#pragma once

#include <optional>
#include <string>

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Vector2.h"

class Game;
class WorldMapMask;

namespace WorldMapUiModel
{
	static constexpr int EXIT_BUTTON_MASK_ID = 9;
	static constexpr int MASK_COUNT = EXIT_BUTTON_MASK_ID + 1;

	std::string getProvinceNameOffsetFilename();

	// Gets the mask click area for a province or the exit button.
	const WorldMapMask &getMask(const Game &game, int maskID);

	// Gets the province ID or exit button ID of the hovered pixel on the world map.
	std::optional<int> getMaskID(Game &game, const Int2 &mousePosition, bool ignoreCenterProvince, bool ignoreExitButton);
}

namespace FastTravelUiModel
{
	// Shortest amount of time the fast travel animation can show for.
	static constexpr double AnimationMinSeconds = 1.0;

	// Advances the game clock after having fast travelled.
	void tickTravelTime(Game &game, int travelDays);

	std::string getCityArrivalMessage(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
}

namespace WorldMapUiView
{
	Int2 getProvinceNameOffset(int provinceID, TextureManager &textureManager);

	TextureAsset getPaletteTextureAsset();
	std::string getProvinceNamesFilename();

	UiTextureID allocHighlightedTextTexture(int provinceID, TextureManager &textureManager, Renderer &renderer);
}

namespace FastTravelUiView
{
	constexpr double AnimationSecondsPerFrame = 1.0 / 24.0;

	Int2 getAnimationTextureCenter();
	std::string getAnimationFilename();
	TextureAsset getPaletteTextureAsset();
}

namespace FastTravelUiController
{
	void onAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays);
}
