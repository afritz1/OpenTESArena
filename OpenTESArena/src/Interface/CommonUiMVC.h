#pragma once

#include <string>

#include "../Input/PointerTypes.h"
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

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	const std::string DebugInfoFontName = ArenaFontName::Arena;
	Color getDebugInfoTextBoxColor();
	constexpr TextAlignment DebugInfoTextAlignment = TextAlignment::TopLeft;
}
