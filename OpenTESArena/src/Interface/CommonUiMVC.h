#ifndef COMMON_UI_MVC_H
#define COMMON_UI_MVC_H

#include <string>

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/UiPivotType.h"
#include "../Utilities/Color.h"

class FontLibrary;
class Renderer;
class TextureManager;

struct Rect;

namespace CommonUiView
{
	constexpr UiPivotType DefaultCursorPivotType = UiPivotType::TopLeft;

	UiTextureID allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer);

	const std::string DebugInfoFontName = ArenaFontName::Arena;
	Color getDebugInfoTextBoxColor();
	constexpr TextAlignment DebugInfoTextAlignment = TextAlignment::TopLeft;
}

#endif
