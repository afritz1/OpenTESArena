#ifndef WORLD_MAP_UI_VIEW_H
#define WORLD_MAP_UI_VIEW_H

#include <string>

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Vector2.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Color.h"

class Game;

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

#endif
