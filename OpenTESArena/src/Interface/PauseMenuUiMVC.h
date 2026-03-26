#ifndef PAUSE_MENU_UI_MVC_H
#define PAUSE_MENU_UI_MVC_H

#include <string>

#include "../Rendering/RenderTextureUtils.h"

class Game;
class Renderer;
class TextureManager;

namespace PauseMenuUiModel
{
	std::string getVolumeString(double percent);
}

namespace PauseMenuUiView
{
	UiTextureID allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

namespace PauseMenuUiController
{
	void onNewGameButtonSelected(Game &game);
}

#endif
