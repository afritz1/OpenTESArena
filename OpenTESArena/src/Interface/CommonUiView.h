#ifndef COMMON_UI_VIEW_H
#define COMMON_UI_VIEW_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/PivotType.h"

class Renderer;
class TextureManager;

namespace CommonUiView
{
	constexpr PivotType DefaultCursorPivotType = PivotType::TopLeft;

	UiTextureID allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
