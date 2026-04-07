#pragma once

#include <string>

class Game;

struct TextureAsset;

namespace MainQuestSplashUiModel
{
	std::string getDungeonText(Game &game, int provinceID);
}

namespace MainQuestSplashUiView
{
	TextureAsset getSplashTextureAsset(int provinceID);
}
