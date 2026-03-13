#ifndef OPTIONS_UI_VIEW_H
#define OPTIONS_UI_VIEW_H

#include "../Rendering/RenderTextureUtils.h"

class Renderer;

namespace OptionsUiView
{
	UiTextureID allocBackgroundTexture(Renderer &renderer);
	UiTextureID allocHighlightTexture(Renderer &renderer);
}

#endif
