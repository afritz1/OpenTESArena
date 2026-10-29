#pragma once

#include <string>

#include "../WorldMap/ProvinceLibrary.h"

class Game;

struct TextureAsset;

namespace MainQuestSplashUiModel
{
	std::string getDungeonText(Game &game, ProvinceDefinitionID provinceDefID);
}

namespace MainQuestSplashUiView
{
	TextureAsset getSplashTextureAsset(ProvinceDefinitionID provinceDefID);
}
