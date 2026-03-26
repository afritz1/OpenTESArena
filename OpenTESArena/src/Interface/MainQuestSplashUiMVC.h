#ifndef MAIN_QUEST_SPLASH_UI_MVC_H
#define MAIN_QUEST_SPLASH_UI_MVC_H

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

#endif
