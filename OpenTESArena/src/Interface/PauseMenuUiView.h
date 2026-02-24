#ifndef PAUSE_MENU_UI_VIEW_H
#define PAUSE_MENU_UI_VIEW_H

#include "../Rendering/RenderTextureUtils.h"

class Renderer;
class TextureManager;

namespace PauseMenuUiView
{
	UiTextureID allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
