#ifndef COMMON_UI_VIEW_H
#define COMMON_UI_VIEW_H

#include <string>

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/UiPivotType.h"
#include "../Utilities/Color.h"

class FontLibrary;
class Renderer;
class TextureManager;

namespace CommonUiView
{
	constexpr UiPivotType DefaultCursorPivotType = UiPivotType::TopLeft;

	UiTextureID allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer);

	const std::string DebugInfoFontName = ArenaFontName::Arena;
	Color getDebugInfoTextBoxColor();
	constexpr TextAlignment DebugInfoTextAlignment = TextAlignment::TopLeft;
	Rect getDebugInfoTextBoxRect();

	TextBoxInitInfo getDebugInfoTextBoxInitInfo(const FontLibrary &fontLibrary);
}

#endif
